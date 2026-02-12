import os
import argparse
import torch
import torch.onnx
import numpy as np
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.distributions import Categorical


class ActorNetwork(nn.Module):
    def __init__(self, device):
        super(ActorNetwork, self).__init__()
        self.device = device
        
        self.conv1 = nn.Sequential(
            nn.Conv2d(4, 16, kernel_size=8, stride=4), 
            nn.LeakyReLU(0.01,inplace=True),
        )
        self.conv2 = nn.Sequential(
            nn.Conv2d(16, 32, kernel_size=4, stride=2), 
            nn.LeakyReLU(0.01,inplace=True),
        )
        self.conv3 = nn.Sequential(
            nn.Conv2d(32, 64, kernel_size=3, stride=1), 
            nn.LeakyReLU(0.01,inplace=True),
        )
        self.features = nn.Sequential(
            nn.Linear(7 * 7 * 64, 256), 
            nn.LeakyReLU(0.01,inplace=True),
        )
        
        self.actor = nn.Linear(256, 2)
        
        self._create_weights()
        
        self.data = []

    def _create_weights(self):
        for m in self.modules():
            if isinstance(m, nn.Conv2d) or isinstance(m, nn.Linear):
                nn.init.uniform_(m.weight, -0.01, 0.01)
                nn.init.constant_(m.bias, 0)

    def forward(self, x):
        x = self.conv1(x)
        x = self.conv2(x)
        x = self.conv3(x)
        x = x.view(x.size(0), -1)
        x = self.features(x)
        return x

    def pi(self, x, softmax_dim=1):
        features = self.forward(x)
        x = self.actor(features)
        prob = F.softmax(x, dim=softmax_dim)
        prob = torch.clamp(prob, 1e-8, 1.0)
        return prob
    
    def put_data(self, trajectory):
        self.data.extend(trajectory)

def convert_to_onnx(input_model_path, output_onnx_path, image_size=84, opset_version=11, dynamic_batch=True, verbose=True):

    try:
        os.environ["CUDA_VISIBLE_DEVICES"] = ""
        torch.set_num_threads(1)
        
        if verbose:
            print(f"Loading PyTorch model from {input_model_path}")
            print(f"Using CPU for all operations")
        
        device = torch.device("cpu")
        
        model = ActorNetwork(device)
        
        checkpoint = torch.load(input_model_path, map_location='cpu')
        
        if 'model_state_dict' in checkpoint:
            state_dict = checkpoint['model_state_dict']
        elif isinstance(checkpoint, dict) and not 'model_state_dict' in checkpoint:
            state_dict = checkpoint
        else:
            state_dict = checkpoint.state_dict()
        
        for k in state_dict:
            state_dict[k] = state_dict[k].cpu() if isinstance(state_dict[k], torch.Tensor) else state_dict[k]
        
        model.load_state_dict(state_dict)
        model.eval()
        
        dummy_input = torch.zeros(1, 4, image_size, image_size, device='cpu')
        
        os.makedirs(os.path.dirname(output_onnx_path), exist_ok=True)
        
        if verbose:
            print(f"Exporting model to ONNX format: {output_onnx_path}")
        
        dynamic_axes = None
        if dynamic_batch:
            dynamic_axes = {'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
        
        class ModelWrapper(torch.nn.Module):
            def __init__(self, model):
                super(ModelWrapper, self).__init__()
                self.model = model
            
            def forward(self, x):
                return self.model.pi(x)
        
        wrapped_model = ModelWrapper(model).to('cpu')
        
        if verbose:
            print("Model prepared for ONNX export on CPU")

        torch.onnx.export(
            wrapped_model,             
            dummy_input,               
            output_onnx_path,          
            export_params=True,        
            opset_version=opset_version,  
            do_constant_folding=True,     
            input_names=['input'],        
            output_names=['output'],      
            dynamic_axes=dynamic_axes     
        )
        
        if verbose:
            print(f"Model successfully exported to: {output_onnx_path}")
            print(f"Input shape: {list(dummy_input.shape)}")
            
        return True
    
    except Exception as e:
        print(f"Error converting model to ONNX: {str(e)}")
        return False

def verify_onnx_model(onnx_path, image_size=84, verbose=True):
    try:
        os.environ["CUDA_VISIBLE_DEVICES"] = ""
        torch.set_num_threads(1)
        
        import onnx
        import onnxruntime as ort
        
        if verbose:
            print(f"Verifying ONNX model: {onnx_path}")
            print(f"Using CPU for all operations")
        
        onnx_model = onnx.load(onnx_path)
        onnx.checker.check_model(onnx_model)
        
        if verbose:
            print("ONNX model structure is valid")
        
        x = np.random.randn(1, 4, image_size, image_size).astype(np.float32)
        
        options = ort.SessionOptions()
        options.intra_op_num_threads = 1
        
        providers = ['CPUExecutionProvider']
        ort_session = ort.InferenceSession(onnx_path, sess_options=options, providers=providers)
        
        ort_inputs = {ort_session.get_inputs()[0].name: x}
        ort_outputs = ort_session.run(None, ort_inputs)
        
        if verbose:
            print(f"ONNX model successfully runs with input shape: {x.shape}")
            print(f"Output shape: {ort_outputs[0].shape}")
            print(f"Output values: min={np.min(ort_outputs[0])}, max={np.max(ort_outputs[0])}")
            print(f"Output example: {ort_outputs[0][0, :2]}") # Print first two probabilities
        
        return True
        
    except Exception as e:
        print(f"Error verifying ONNX model: {str(e)}")
        return False

def get_args():
    parser = argparse.ArgumentParser(description="Convert PyTorch model to ONNX format")
    parser.add_argument("--input_model", type=str, default="trained_models/flappy_bird_latest.pth",
                        help="Path to the input PyTorch model")
    parser.add_argument("--output_onnx", type=str, default="trained_models/flappy_bird_rl.onnx",
                        help="Path to save the ONNX model")
    parser.add_argument("--image_size", type=int, default=84,
                        help="Input image size (default: 84x84)")
    parser.add_argument("--opset_version", type=int, default=15,
                        help="ONNX opset version")
    parser.add_argument("--verify", action="store_true",
                        help="Verify the ONNX model after conversion")
    parser.add_argument("--static_batch", action="store_true",
                        help="Use static batch size (default: dynamic)")
    
    return parser.parse_args()

if __name__ == "__main__":
    args = get_args()
    
    success = convert_to_onnx(
        args.input_model,
        args.output_onnx,
        args.image_size,
        args.opset_version,
        not args.static_batch
    )
    
    if success and args.verify:
        verify_onnx_model(args.output_onnx, args.image_size) 