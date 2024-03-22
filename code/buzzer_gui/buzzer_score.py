import zmq
import argparse
import json
import os
import dearpygui.dearpygui as dpg

score_values = [0, 0, 0, 0, 0]  # Initial score values for 5 groups

def load_json_file(file_path):
    if not os.path.exists(file_path):
        return {}
    with open(file_path, 'r') as f:
        return json.load(f)

def update_score(group_num, points):
    global score_values
    if 1 <= group_num <= 5:
        score_values[group_num-1] += points
        dpg.configure_item(f"Team {group_num}", text=f"Team {group_num}")
        dpg.configure_item(f"Teamscore {group_num}", text=str(score_values[group_num-1]))

def main(args):
    context = zmq.Context()
    socket_score = context.socket(zmq.PULL)
    socket_score.connect("tcp://127.0.0.1:5556")
    
    score_config = load_json_file(args.score_config)
    
    for i in range(1,6):
        score_values[i-1] = score_config["group_initial_score"][str(i)]

    width = score_config["width"]
    height = score_config["height"]
    x_pos = score_config["x_pos"]
    y_pos = score_config["y_pos"]
    
    dpg.create_context()
    dpg.create_viewport(title='Scoreboard', width=width, height=height, x_pos=x_pos, y_pos=y_pos)
    dpg.setup_dearpygui()
    
    # Add scoreboard elements
    with dpg.window(label="Scoreboard", tag="buzzer_score", pos=(0, 700), width=1400, height=50, no_close=True):
        with dpg.group(horizontal=True, horizontal_spacing=10):
            for i in range(1,6):
                with dpg.drawlist(width=270, height=50, pos=(200*(i), 0)):
                    dpg.draw_rectangle((0, 0), (240, 50))
                    dpg.draw_text((50, 0), f"Team {i}", size=15, tag=f'Team {i}')
                    dpg.draw_text((50, 10), str(score_values[i-1]), size=40, tag=f"Teamscore {i}")
    
    dpg.show_viewport()

    while dpg.is_dearpygui_running():
        message = socket_score.recv_string()
        group_num, points = message.split(",")
        group_num = int(group_num)
        points = int(points)
        update_score(group_num, points)
        
        dpg.render_dearpygui_frame()

    dpg.destroy_context()

if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Scoreboard GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args.add_argument("--score_config", type=str, default="buzzer_score_config.json", help="Path to the JSON file containing the scoreboard position")
    args = args.parse_args()
    main(args)
