from itertools import cycle
from numpy.random import randint
import os
from pygame import Rect, init, time, display
from pygame.event import pump
from pygame.image import load
from pygame.surfarray import array3d, pixels_alpha
from pygame.transform import rotate
import numpy as np
import cv2

class FlappyBird(object):
    def __init__(self, headless=False, pipe_gap_size=200):
        if headless:
            os.environ['SDL_VIDEODRIVER'] = 'dummy'
        
        if not hasattr(FlappyBird, 'initialized'):
            init()
            FlappyBird.initialized = True
            
            FlappyBird.fps_clock = time.Clock()
            FlappyBird.screen_width = 288
            FlappyBird.screen_height = 512
            FlappyBird.screen = display.set_mode((FlappyBird.screen_width, FlappyBird.screen_height))
            display.set_caption('Flappy Bird RL')
            
            FlappyBird.base_image = load('assets/sprites/base.png').convert_alpha()
            FlappyBird.background_image = load('assets/sprites/background-black.png').convert()
            
            FlappyBird.pipe_images = [
                rotate(load('assets/sprites/pipe-green.png').convert_alpha(), 180),
                load('assets/sprites/pipe-green.png').convert_alpha()
            ]
            
            FlappyBird.bird_images = [
                load('assets/sprites/redbird-upflap.png').convert_alpha(),
                load('assets/sprites/redbird-midflap.png').convert_alpha(),
                load('assets/sprites/redbird-downflap.png').convert_alpha()
            ]
            
            FlappyBird.bird_hitmask = [pixels_alpha(image).astype(bool) for image in FlappyBird.bird_images]
            FlappyBird.pipe_hitmask = [pixels_alpha(image).astype(bool) for image in FlappyBird.pipe_images]
        
        self.fps = 30
        self.pipe_gap_size = pipe_gap_size
        self.pipe_velocity_x = -4
        
        self.min_velocity_y = -8
        self.max_velocity_y = 10
        self.downward_speed = 1
        self.upward_speed = -9
        
        self.bird_index_generator = cycle([0, 1, 2, 1])
        self.iter = 0
        self.bird_index = 0
        self.score = 0
        
        self.bird_width = FlappyBird.bird_images[0].get_width()
        self.bird_height = FlappyBird.bird_images[0].get_height()
        self.pipe_width = FlappyBird.pipe_images[0].get_width()
        self.pipe_height = FlappyBird.pipe_images[0].get_height()
        

        self.bird_x = int(FlappyBird.screen_width / 5) + np.random.randint(-20,20)

        self.bird_y = int((FlappyBird.screen_height - self.bird_height) / 2) + np.random.randint(-15,15)
        self.base_x = 0
        self.base_y = FlappyBird.screen_height * 0.79
        self.base_shift = FlappyBird.base_image.get_width() - FlappyBird.background_image.get_width()
        
        pipes = [self.generate_pipe(), self.generate_pipe()]
        pipes[0]["x_upper"] = pipes[0]["x_lower"] = FlappyBird.screen_width * 1.0
        pipes[1]["x_upper"] = pipes[1]["x_lower"] = FlappyBird.screen_width * 1.5
        self.pipes = pipes
        
        self.current_velocity_y = 0
        self.is_flapped = False

    def generate_pipe(self):
        x = FlappyBird.screen_width
        gap_y = randint(2, 10) * 10 + int(self.base_y / 5)
        return {"x_upper": x, "y_upper": gap_y - self.pipe_height, "x_lower": x, "y_lower": gap_y + self.pipe_gap_size}

    def is_collided(self):
        if self.bird_height + self.bird_y + 1 >= self.base_y:
            return True, "down"
        if self.bird_y <= 0:
            return True, "up"
            
        bird_bbox = Rect(self.bird_x, self.bird_y, self.bird_width, self.bird_height)
        
        for pipe in self.pipes:
            upper_pipe_rect = Rect(pipe["x_upper"], pipe["y_upper"], self.pipe_width, self.pipe_height)
            lower_pipe_rect = Rect(pipe["x_lower"], pipe["y_lower"], self.pipe_width, self.pipe_height)
            
            if bird_bbox.colliderect(upper_pipe_rect):
                cropped_bbox = bird_bbox.clip(upper_pipe_rect)
                
                min_x1 = cropped_bbox.x - bird_bbox.x
                min_y1 = cropped_bbox.y - bird_bbox.y
                min_x2 = cropped_bbox.x - upper_pipe_rect.x
                min_y2 = cropped_bbox.y - upper_pipe_rect.y
                
                if (min_x1 >= 0 and min_y1 >= 0 and min_x2 >= 0 and min_y2 >= 0 and
                    min_x1 + cropped_bbox.width <= FlappyBird.bird_hitmask[self.bird_index].shape[0] and
                    min_y1 + cropped_bbox.height <= FlappyBird.bird_hitmask[self.bird_index].shape[1] and
                    min_x2 + cropped_bbox.width <= FlappyBird.pipe_hitmask[0].shape[0] and
                    min_y2 + cropped_bbox.height <= FlappyBird.pipe_hitmask[0].shape[1]):
                    #print(self.bird_index)
                    
                    if np.any(FlappyBird.bird_hitmask[self.bird_index][min_x1:min_x1 + cropped_bbox.width,
                           min_y1:min_y1 + cropped_bbox.height] * FlappyBird.pipe_hitmask[0][min_x2:min_x2 + cropped_bbox.width,
                                                                   min_y2:min_y2 + cropped_bbox.height]):
                        return True, "down"
                else:
                    return True, "down"
            
            if bird_bbox.colliderect(lower_pipe_rect):
                cropped_bbox = bird_bbox.clip(lower_pipe_rect)
                
                min_x1 = cropped_bbox.x - bird_bbox.x
                min_y1 = cropped_bbox.y - bird_bbox.y
                min_x2 = cropped_bbox.x - lower_pipe_rect.x
                min_y2 = cropped_bbox.y - lower_pipe_rect.y
                
                if (min_x1 >= 0 and min_y1 >= 0 and min_x2 >= 0 and min_y2 >= 0 and
                    min_x1 + cropped_bbox.width <= FlappyBird.bird_hitmask[self.bird_index].shape[0] and
                    min_y1 + cropped_bbox.height <= FlappyBird.bird_hitmask[self.bird_index].shape[1] and
                    min_x2 + cropped_bbox.width <= FlappyBird.pipe_hitmask[1].shape[0] and
                    min_y2 + cropped_bbox.height <= FlappyBird.pipe_hitmask[1].shape[1]):
                    
                    if np.any(FlappyBird.bird_hitmask[self.bird_index][min_x1:min_x1 + cropped_bbox.width,
                           min_y1:min_y1 + cropped_bbox.height] * FlappyBird.pipe_hitmask[1][min_x2:min_x2 + cropped_bbox.width,
                                                                   min_y2:min_y2 + cropped_bbox.height]):
                        return True, "down"
                else:
                    return True, "down"
            
        return False, "none"

    def next_frame(self, action):
        pump()
        reward = 0.01
        terminal = False
        
        if action == 1:
            self.current_velocity_y = self.upward_speed
            self.is_flapped = True

        bird_center_x = self.bird_x + self.bird_width / 2
        for pipe in self.pipes:
            pipe_center_x = pipe["x_upper"] + self.pipe_width / 2
            if pipe_center_x < bird_center_x < pipe_center_x + 5:
                self.score += 1
                reward = 1
                break

        self.bird_index = next(self.bird_index_generator)
            
        self.base_x = -((-self.base_x + 100) % self.base_shift)

        if self.current_velocity_y < self.max_velocity_y and not self.is_flapped:
            self.current_velocity_y += self.downward_speed
        if self.is_flapped:
            self.is_flapped = False
        self.bird_y = max(0, self.bird_y + self.current_velocity_y)

        for pipe in self.pipes:
            pipe["x_upper"] += self.pipe_velocity_x
            pipe["x_lower"] += self.pipe_velocity_x
            
        if 0 < self.pipes[0]["x_lower"] < 5:
            self.pipes.append(self.generate_pipe())
        if self.pipes[0]["x_lower"] < -self.pipe_width:
            del self.pipes[0]
        k = self.is_collided()
        if k[0] == True and k[1] == "up":
            terminal = True
            reward = -3
            self.reset_game_state()
        elif k[0] == True and k[1] == "down":
            terminal = True
            reward = -2
            self.reset_game_state()

        FlappyBird.screen.blit(FlappyBird.background_image, (0, 0))
        FlappyBird.screen.blit(FlappyBird.base_image, (self.base_x, self.base_y))
        FlappyBird.screen.blit(FlappyBird.bird_images[self.bird_index], (self.bird_x, self.bird_y))
        for pipe in self.pipes:
            FlappyBird.screen.blit(FlappyBird.pipe_images[0], (pipe["x_upper"], pipe["y_upper"]))
            FlappyBird.screen.blit(FlappyBird.pipe_images[1], (pipe["x_lower"], pipe["y_lower"]))
        
        image = array3d(display.get_surface())
        image = np.flipud(np.rot90(image))
        
        display.update()
        FlappyBird.fps_clock.tick(self.fps)
        
        return image, reward, terminal
    
    def reset_game_state(self):
        self.iter = 0
        self.bird_index = 0
        self.score = 0
        
        self.bird_x = int(FlappyBird.screen_width / 5) + np.random.randint(-20,20)
        self.bird_y = int((FlappyBird.screen_height - self.bird_height) / 2) + np.random.randint(-15, 15)
        
        self.base_x = 0
        
        pipes = [self.generate_pipe(), self.generate_pipe()]
        pipes[0]["x_upper"] = pipes[0]["x_lower"] = FlappyBird.screen_width * 1.0
        pipes[1]["x_upper"] = pipes[1]["x_lower"] = FlappyBird.screen_width * 1.5
        self.pipes = pipes
        
        self.current_velocity_y = 0
        self.is_flapped = False
        
    def set_pipe_gap_size(self, gap_size):
        self.pipe_gap_size = gap_size
        
    def set_pipe_velocity(self, velocity):
        self.pipe_velocity_x = velocity


