import zmq
import argparse
import json
import os
import cv2
import time

def load_json_file(file_path):
    if not os.path.exists(file_path):
        return {}
    with open(file_path, 'r') as f:
        return json.load(f)

def play_video(condition):
    # Check if mp4 json is correctly loaded
    if condition not in mp4_config["mp4_paths"]:
        print(f"Video path for condition {condition} not found")
        return

    video_path = os.path.expanduser(mp4_config["mp4_paths"][condition])

    if not os.path.exists(video_path):
        print(f"Video path for condition {condition} does not exist")
        return

    from ffpyplayer.player import MediaPlayer

    video = cv2.VideoCapture(video_path)
    player = MediaPlayer(video_path)

    width = mp4_config["width"]
    height = mp4_config["height"]
    x_pos = mp4_config["x_pos"]
    y_pos = mp4_config["y_pos"]

    cv2.namedWindow("Winner", cv2.WINDOW_GUI_NORMAL)
    cv2.resizeWindow("Winner", width, height)
    cv2.moveWindow("Winner", x_pos, y_pos)

    start_time = time.time()

    while True:
        ret, frame = video.read()
        if not ret:
            break
        if not player.get_frame():
            break

        cv2.imshow("Winner", frame)

        # Sync video with audio
        elapsed_time = (time.time() - start_time) * 1000 # in ms
        play_time = video.get(cv2.CAP_PROP_POS_MSEC)
        sleep = max(1, int(play_time - elapsed_time))

        if cv2.waitKey(sleep) & 0xFF == ord('q'):
            break

    player.close_player()
    video.release()
    cv2.destroyAllWindows()


def main(args):
    global mp4_config
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.connect("tcp://127.0.0.1:5555")

    mp4_config = load_json_file(args.mp4_config)

    # magic, do not touch
    cv2.namedWindow("Winner", cv2.WINDOW_GUI_NORMAL)
    cv2.destroyAllWindows()
    # end of magic

    while True:
        message = socket.recv_string()
        print("Received: ", message)
        play_video(message)

if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Buzzer GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args.add_argument("--mp4_config", type=str, default="buzzer_mp4_config.json", help="Path to the JSON file containing the mp4 paths for different conditions")
    args = args.parse_args()
    main(args)
