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
    dpg.create_viewport(title='Scoreboard', width=width, height=height, x_pos=x_pos, y_pos=y_pos, always_on_top=True, resizable=False)
    dpg.setup_dearpygui()

    image_width, image_height, image_channel, image_data = dpg.load_image("./assets/design/carnival4.jpeg")

    with dpg.texture_registry(show=True):
        dpg.add_static_texture(default_value=image_data, width=image_width, height=image_height, tag="carnival_texture")


    # Add scoreboard elements
    with dpg.window(label="Scoreboard", tag="buzzer_score", width=width, height=height, no_collapse=True, no_move=True, no_resize=True, no_title_bar=True):
        for i in range(5):
            dpg.add_image(texture_tag="carnival_texture", pos=(100+350*i, 10), width=240, height=height-20, tag=f"Image {i+1}")
            dpg.draw_text((130+350*i, 0), f"Team {i+1}", size=50, tag=f'Team {i+1}', color=(0, 0, 0))
            dpg.draw_text((185+350*i, 90), str(score_values[i-1]), size=60, tag=f"Teamscore {i+1}", color=(0, 0, 0))


    dpg.show_viewport()

    while dpg.is_dearpygui_running():
        try:
            message = socket_score.recv_string(flags=zmq.NOBLOCK)
            group_num, points = message.split(",")
            group_num = int(group_num)
            points = int(points)
            update_score(group_num, points)
        except zmq.Again as e:
            pass

        dpg.render_dearpygui_frame()

    dpg.destroy_context()

if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Scoreboard GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args.add_argument("--score_config", type=str, default="buzzer_score_config.json", help="Path to the JSON file containing the scoreboard position")
    args = args.parse_args()
    main(args)
