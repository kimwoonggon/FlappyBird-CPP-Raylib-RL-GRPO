import argparse
from random import random, randint
import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from torch.distributions import Categorical
import cv2
from flappy_bird_init_rewardset import FlappyBird
from torch.utils.data import TensorDataset, DataLoader
from tqdm import tqdm
import copy
import os

DEBUG = False


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

def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image_size", type=int, default=84, help="Common width and height for all images")
    parser.add_argument("--batch_size", type=int, default=32, help="Number of samples per mini-batch")
    parser.add_argument("--gradient_accumulation_steps", type=int, default=1, help="Number of steps to accumulate gradients")
    parser.add_argument("--lr", type=float, default=1e-4, help="Learning rate")
    parser.add_argument("--gamma", type=float, default=0.99, help="Discount factor")
    parser.add_argument("--eps_clip", type=float, default=0.1, help="Clip parameter")
    parser.add_argument("--K_epoch", type=int, default=3, help="Number of policy update epochs")
    parser.add_argument("--T_horizon", type=int, default=2048, help="Maximum trajectory length")
    parser.add_argument("--num_rollouts", type=int, default=2, help="Number of rollouts to collect before training")
    parser.add_argument("--kl_weight", type=float, default=0.1, help="KL divergence weight")
    parser.add_argument("--num_episodes", type=int, default=10000, help="Number of episodes to train")
    parser.add_argument("--saved_path", type=str, default="trained_models")
    parser.add_argument("--model_checkpoint", type=str, default="flappy_bird_lat.pth", help="Checkpoint file to load (set to None for no loading)")
    parser.add_argument("--headless", action="store_true", help="Run in headless mode")
    parser.add_argument("--reward_scale", type=float, default=1.0, help="Reward scaling factor")
    parser.add_argument("--use_cuda", type=bool, default=True, help="Use CUDA if available")
    parser.add_argument("--entropy_coef", type=float, default=0.005, help="Entropy coefficient")
    parser.add_argument("--max_grad_norm", type=float, default=0.5, help="Max gradient norm for clipping")
    parser.add_argument("--time_reward_scale", type=float, default=0.001, help="Scale factor for time-based reward")
    parser.add_argument("--initial_gap_size", type=int, default=200, help="Initial pipe gap size")
    return parser.parse_args()

def pre_processing(image, width, height):
    image = cv2.cvtColor(cv2.resize(image, (width, height)), cv2.COLOR_BGR2GRAY)
    _, image = cv2.threshold(image, 1, 255, cv2.THRESH_BINARY)
    return image[None, :, :].astype(np.float32)

def collect_trajectory(game_state, model, device, opt, action_counts, episode_action_counts=None):
    image, reward, terminal = game_state.next_frame(0)
    image = pre_processing(image[:int(game_state.base_y), :], opt.image_size, opt.image_size)
    
    image = torch.from_numpy(image).to(device)
    state = torch.cat(tuple(image for _ in range(4)))[None, :, :, :]
    
    trajectory = []
    total_reward = 0
    total_time_reward = 0
    
    for t in range(opt.T_horizon):
        with torch.no_grad():
            prob = model.pi(state)
        dist = Categorical(prob)
        
        action = dist.sample().item()
        action_prob = prob[0][action].item()
        
        action_counts[action] += 1
        if episode_action_counts is not None:
            episode_action_counts[action] += 1
        
        next_image, reward, terminal = game_state.next_frame(action)
        
        time_reward = 0
        reward += time_reward
        total_time_reward += time_reward
        
        next_image = pre_processing(next_image[:int(game_state.base_y), :], opt.image_size, opt.image_size)
        next_image = torch.from_numpy(next_image).to(device)
        next_state = torch.cat((state[0, 1:, :, :], next_image))[None, :, :, :]
        
        trajectory.append((state, action, reward, next_state, action_prob, terminal))
        
        state = next_state
        total_reward += reward
        
        if terminal:
            break
    
    return trajectory, total_reward, total_time_reward, terminal, t+1

def compute_returns(rewards, gamma):
    returns = []
    R = 0
    for r in reversed(rewards):
        R = r + gamma * R
        returns.insert(0, R)
    return returns
    # [::-1] 하면 끝단의 점수가 앞으로 오기 때문에 안맞는 듯
    #return returns[::-1]

def train(opt):
    torch.autograd.set_detect_anomaly(True)
    
    use_cuda = opt.use_cuda and torch.cuda.is_available()
    device = torch.device("cuda" if use_cuda else "cpu")
    print(f"Using device: {device}")
    
    if use_cuda:
        torch.cuda.manual_seed(123)
    else:
        torch.manual_seed(123)
    
    model = ActorNetwork(device)
    model.to(device)
    
    ref_model = ActorNetwork(device)
    ref_model.to(device)
    
    start_episode = 0
    running_reward = 0
    entropy_history = []
    policy_loss_history = []
    kl_div_history = []
    action_counts = [0, 0]
    time_reward_history = []
    current_gap_size = opt.initial_gap_size
    
    if opt.model_checkpoint:
        checkpoint_path = os.path.join(opt.saved_path, opt.model_checkpoint)
        if os.path.exists(checkpoint_path):
            print(f"Loading checkpoint from {checkpoint_path}")
            checkpoint = torch.load(checkpoint_path, map_location=device)
            model.load_state_dict(checkpoint['model_state_dict'])
            
            start_episode = checkpoint.get('episode', 0) + 1
            running_reward = checkpoint.get('running_reward', 0)
            
            entropy_history = checkpoint.get('entropy_history', [])
            policy_loss_history = checkpoint.get('policy_loss_history', [])
            kl_div_history = checkpoint.get('kl_div_history', [])
            action_counts = checkpoint.get('action_counts', [0, 0])
            time_reward_history = checkpoint.get('time_reward_history', [])
            current_gap_size = checkpoint.get('current_gap_size', opt.initial_gap_size)
            
            print(f"Checkpoint loaded! Continuing from episode {start_episode}")
            print(f"Current running reward: {running_reward:.2f}")
            print(f"Current gap size: {current_gap_size}")
        else:
            print(f"Checkpoint {checkpoint_path} not found. Starting new training.")
    
    ref_model.load_state_dict(model.state_dict())
    
    optimizer = optim.Adam(model.parameters(), lr=opt.lr)
    
    if opt.model_checkpoint and os.path.exists(checkpoint_path) and 'optimizer_state_dict' in checkpoint:
        print("optimizer loaded")
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        print("Optimizer state loaded from checkpoint")
        optimizer.param_groups[0]['lr'] = opt.lr
    
    main_game_state = FlappyBird(headless=opt.headless, pipe_gap_size=current_gap_size)
    
    score = 0.0
    print_interval = 20
    scores = []
    episode_rewards = []
    max_grad_norm = opt.max_grad_norm
    avg_reward_history = []
    
    episode_action_counts = [0, 0]
    
    os.makedirs(opt.saved_path, exist_ok=True)
    
    total_steps = 0
    n_epi = start_episode
    
    pbar = tqdm(total=opt.num_episodes, initial=start_episode, desc="Episodes")
    
    while n_epi < start_episode + opt.num_episodes:
        gap_size = 200
        opt.num_rollouts = 6
        if n_epi >= 300 and n_epi < 400:
            gap_size = 150
            opt.num_rollouts = 6
        if n_epi >= 400 and n_epi < 500:
            opt.num_rollouts = 2
            gap_size=150
                
        elif n_epi >= 500 and n_epi < 700:
            gap_size = 100
            opt.num_rollouts = 6
        elif n_epi >= 700 and n_epi < 1300:
                opt.num_rollouts = 6
                gap_size=100
        elif n_epi >= 1300:
            gap_size = 100
            opt.num_rollouts = 2
            #opt.num_rollouts = 10
            
        if gap_size != current_gap_size:
            main_game_state.set_pipe_gap_size(gap_size)
            current_gap_size = gap_size
            print(f"\nDifficulty adjusted: Pipe gap size is now {gap_size}")
        
        all_trajectories = []
        all_rewards = []

        all_time_rewards = []
        all_terminals = []
        all_steps = []
        
        episode_completed = False
        len_total_trajectories = 0
        for i in range(opt.num_rollouts):
            trajectory, reward, time_reward, terminal, steps = collect_trajectory(
                main_game_state, ref_model, device, opt, action_counts, 
                episode_action_counts if i == 0 else None
            )
            len_total_trajectories += len(trajectory)
            all_trajectories.append(trajectory)
            all_rewards.append(reward)
            all_time_rewards.append(time_reward)
            all_terminals.append(terminal)
            all_steps.append(steps)
            print("trajectory gathered : ", len(trajectory))
            if i == 0:
                    n_epi += 1
                    pbar.update(1)
                    
                    score += reward
                    scores.append(reward)
                    episode_rewards.append(reward)
                    time_reward_history.append(time_reward)
                    running_reward = 0.05 * reward + 0.95 * running_reward
                    
                    total_episode_actions = sum(episode_action_counts)
                    flap_ratio = episode_action_counts[1] / total_episode_actions if total_episode_actions > 0 else 0
                    
                    print(f"\nEpisode {n_epi} finished after {steps} steps.")
                    print(f"Reward: {reward:.2f} (Time reward: {time_reward:.2f}), Running reward: {running_reward:.2f}")
                    print(f"Current difficulty: Gap size = {current_gap_size}")
                    if 'avg_entropy' in locals() and 'avg_policy_loss' in locals() and 'avg_kl_div' in locals():
                        print(f"Entropy: {avg_entropy:.3f}, Policy Loss: {avg_policy_loss:.3f}, KL Div: {avg_kl_div:.3f}")
                    print(f"Episode actions: [No-flap: {episode_action_counts[0]}, Flap: {episode_action_counts[1]}], Flap ratio: {flap_ratio:.2f}")
                    print(f"Total actions: [No-flap: {action_counts[0]}, Flap: {action_counts[1]}], Overall flap ratio: {action_counts[1]/sum(action_counts):.2f}")
                    
                    episode_action_counts[0] = 0
                    episode_action_counts[1] = 0
                    
            total_steps += steps
        
        inner_group_advantage_list = []
        group_advantage_list = []
        total_policy_loss = 0
        total_entropy = 0
        total_kl_div = 0
        tot_s_batch = []
        tot_a_batch = []
        tot_prob_batch = []
        
        for trajectory in all_trajectories:
            states = []
            actions = []
            rewards = []
            probs = []
            
            for s, a, r, _, prob_a, _ in trajectory:
                states.append(s)
                actions.append(a)
                rewards.append(r)
                probs.append(prob_a)
            
            returns = compute_returns(rewards, opt.gamma)
            
            s_batch = torch.cat(states)
            s_batch = s_batch.to(device) if not s_batch.is_cuda else s_batch
            a_batch = torch.tensor([[a] for a in actions]).to(device)
            returns_batch = torch.tensor([[r] for r in returns], dtype=torch.float).to(device)
            prob_batch = torch.tensor([[p] for p in probs], dtype=torch.float).to(device)
            tot_s_batch.append(s_batch)
            tot_a_batch.append(a_batch)
            tot_prob_batch.append(prob_batch)
            
            trajectory_returns = returns_batch.view(-1)
            trajectory_mean = trajectory_returns.mean()
            trajectory_std = trajectory_returns.std()
            
            if trajectory_std < 1e-6:
                normalized_returns = trajectory_returns - trajectory_mean
            else:
                normalized_returns = (trajectory_returns - trajectory_mean) / (trajectory_std + 1e-8)
            normalized_returns = torch.clamp(normalized_returns, -10.0, 10.0)
            
            inner_group_advantage_list.append(normalized_returns)
            
        group_advantages = inner_group_advantage_list
        for epoch in range(opt.K_epoch):
            group_loss = 0
            for groupidx, (s_batch, a_batch, prob_batch, normalized_returns) in enumerate(zip(tot_s_batch, tot_a_batch, tot_prob_batch, group_advantages)):
                pi = model.pi(s_batch)
                dist = Categorical(pi)
                
                with torch.no_grad():
                    ref_pi = ref_model.pi(s_batch)
                
                log_pi_a = dist.log_prob(a_batch.squeeze()).unsqueeze(1)
                log_prob_a = torch.log(prob_batch + 1e-8)
                ratio = torch.exp(log_pi_a - log_prob_a)
                
                
                surr1 = ratio * normalized_returns.unsqueeze(1)
                surr2 = torch.clamp(ratio, 1 - opt.eps_clip, 1 + opt.eps_clip) * normalized_returns.unsqueeze(1)
                
                policy_loss = -torch.min(surr1, surr2)
                
                entropy = dist.entropy().unsqueeze(1)
                
                pi_a = pi.gather(1, a_batch)
                with torch.no_grad():
                    ref_pi_a = ref_pi.gather(1, a_batch)
                
                log_pi_a = torch.log(pi_a + 1e-8)
                log_ref_pi_a = torch.log(ref_pi_a + 1e-8)
                log_ratio = log_ref_pi_a - log_pi_a
                kl_loss = torch.exp(log_ratio) - log_ratio - 1
                kl_div = kl_loss.mean()
                
                loss = (policy_loss - 
                    opt.entropy_coef * entropy +
                    opt.kl_weight * kl_loss) / opt.gradient_accumulation_steps
                group_loss += loss.mean()
                
            if torch.isnan(group_loss).any():
                print("Warning: NaN detected. Skipping backward pass.")
                optimizer.zero_grad()
            else:
                group_loss.backward()
                
            nn.utils.clip_grad_norm_(model.parameters(), max_grad_norm)
            optimizer.step()
            optimizer.zero_grad()
            

        current_lr = optimizer.param_groups[0]['lr']
        

        
        print(f"\nTraining Update Summary:")
        print(f"Current Learning Rate: {current_lr:.7f}")
        print(f"Collected {len(all_trajectories)} trajectories with {len_total_trajectories} total steps")
        del s_batch, a_batch, returns_batch, prob_batch, pi, dist, ref_pi, loss
        del tot_s_batch, tot_a_batch, tot_prob_batch, group_advantages
        del all_trajectories, all_rewards, all_time_rewards, all_terminals, all_steps
        if torch.cuda.is_available():
            torch.cuda.empty_cache()
        
        if n_epi % print_interval == 0 and n_epi != 0:
            avg_score = score / print_interval
            avg_reward_history.append(avg_score)
            
            print(f"\nProgress Report - Episode: {n_epi}")
            print(f"Average reward (last {print_interval} episodes): {avg_score:.1f}")
            print(f"Current difficulty: Gap size = {current_gap_size}")
            print(f"Learning rate: {current_lr:.6f}")
            print(f"Action distribution: [No-flap: {action_counts[0]}, Flap: {action_counts[1]}], Flap ratio: {action_counts[1]/sum(action_counts):.2f}")
            
            score = 0.0
        
        if n_epi % 5 == 0 and n_epi != 0:
            checkpoint_data = {
                'episode': n_epi,
                'model_state_dict': model.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(),
                'running_reward': running_reward,
                'entropy_history': entropy_history,
                'policy_loss_history': policy_loss_history,
                'kl_div_history': kl_div_history,
                'action_counts': action_counts,
                'time_reward_history': time_reward_history,
                'current_gap_size': current_gap_size
            }
            
            torch.save(checkpoint_data, f"{opt.saved_path}/flappy_bird_{n_epi}.pth")
            
            torch.save(checkpoint_data, f"{opt.saved_path}/flappy_bird_latest.pth")
        
        if n_epi % 1 == 0 and n_epi != 0:
            ref_model.load_state_dict(model.state_dict())
        
        if n_epi > 500 and n_epi % 100 == 0:
            recent_rewards = avg_reward_history[-5:]
            recent_entropy = np.mean(entropy_history[-100:])
            
            if len(recent_rewards) >= 5 and all(r < 1.0 for r in recent_rewards) and recent_entropy < 0.1:
                print("Early stopping: Low rewards and entropy indicate training has stagnated")
                break
    
    pbar.close()

if __name__ == "__main__":
    opt = get_args()
    train(opt)
