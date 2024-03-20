import zmq
import argparse
import json
import os
import cv2
import dearpygui.dearpygui as dpg

score_values = [0, 0, 0, 0, 0]  # Initial score values for 5 groups

def create_scoreboard():
    with dpg.window(label="Scoreboard", tag="buzzer_score", pos=(0, 700), width=1280, height=50, no_close=True):
        with dpg.group(horizontal=True, horizontal_spacing=10):
            for i in range(1,6):
                with dpg.drawlist(width=240, height=50, pos=(200*(i), 0)):
                    dpg.draw_rectangle((0, 0), (240, 50))
                    dpg.draw_text((50, 0), f"Team {i}", size=15, tag=f'Team {i}')
                    dpg.draw_text((50, 10), str(score_values[i-1]), size=30, tag=f"Teamscore {i}")

def update_score(group_num, increment):
    global score_values
    if 1 <= group_num <= 5:
        score_values[group_num-1] += increment
        # Update the score text
        dpg.configure_item(f"Team {group_num}", text=f"Team {group_num}")
        dpg.configure_item(f"Teamscore {group_num}", text=str(score_values[group_num-1]))

def main(args):
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.connect("tcp://127.0.0.1:5555")
        
    dpg.create_context()
    dpg.create_viewport(title='Scoreboard', width=1920, height=800, x_pos=1921, y_pos=0)
    dpg.setup_dearpygui()
    
    # Add scoreboard elements
    create_scoreboard()
    
    dpg.show_viewport()

    while dpg.is_dearpygui_running():
        message = socket.recv_string()
        group_num, increment = message.split(",")
        group_num = int(group_num)
        increment = int(increment)
        #group_num = 2 debug
        #increment = 1
        update_score(group_num, increment)
        dpg.render_dearpygui_frame()

    dpg.destroy_context()

if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Scoreboard GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args = args.parse_args()
    main(args)
