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

    fps = mp4_config["fps"]
    width = mp4_config["width"]
    height = mp4_config["height"]
    x_pos = mp4_config["x_pos"]
    y_pos = mp4_config["y_pos"]

    # cv2.namedWindow("Winner", cv2.WINDOW_GUI_NORMAL)
    # cv2.namedWindow("Winner" )
    # print("hi")
    # cv2.resizeWindow("Winner", width, height)
    # cv2.moveWindow("Winner", x_pos, y_pos)
    player = MediaPlayer(video_path)
    val = ''

    while val != 'eof':
        ret, frame = video.read()
        audio_frame, val = player.get_frame()
        print(val)
        if not ret:
            print("Reached end of video")
            break
        if cv2.waitKey(int(1000/fps)) == ord("q"):
            break
        cv2.imshow("Winner", frame)
        if val != 'eof' and audio_frame is not None:
            #audio
            img, t = audio_frame

        # run at fps
        time.sleep(1/fps)

    video.release()
    cv2.destroyAllWindows()


def main(args):
    global mp4_config
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.connect("tcp://127.0.0.1:5555")

    mp4_config = load_json_file(args.mp4_config)

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
