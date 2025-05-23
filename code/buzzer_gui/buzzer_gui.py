import serial
import serial.tools.list_ports
import dearpygui.dearpygui as dpg
import ast
from logger import mvLogger
import argparse
import zmq
import re

serial_port = None  # Store the serial port object
winner_id = 0
lock = False # Initialized with Locked

online_bg_color = (128, 128, 128)
offline_bg_color = (50, 50, 50)
online_bg_color = (128, 128, 128)
winner_bg_color = (0, 255, 0)

buttons_info = {}

context = zmq.Context()
#mp4
socket = context.socket(zmq.PUSH)
socket.bind("tcp://127.0.0.1:5555")
#scoreboard
socket_score = context.socket(zmq.PUSH)
socket_score.bind("tcp://127.0.0.1:5556")


def play_video(condition):
    socket.send_string(condition)

def update_score(group_num, increment):
    socket_score.send_string(f"{group_num},{increment}")

def log_info(message):
    global logx
    logx.log_info(message)

def log_debug(message):
    global logx
    logx.log_debug(message)

def log_warning(message):
    global logx
    logx.log_warning(message)

def log_error(message):
    global logx
    logx.log_error(message)


def find_esp32_ports():
    ports_found = {}  # Create a dictionary to store the COM port of the ESP32 device
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "Espressif" in str(port.manufacturer):
            ports_found[port.device] = port.serial_number
    return ports_found

def on_port_select(sender):
    selected_port = dpg.get_value(sender)
    log_info(f"Selected port: {selected_port}")

def on_port_select_button_click():
    global serial_port

    if serial_port is not None and serial_port.is_open:
        log_info("Serial port already opened")
        return

    selected_port = dpg.get_value("port_selection") or "(None,None)"
    # Convert the selected port from a string back to a tuple
    selected_port = ast.literal_eval(selected_port)[0] or ""

    if selected_port == "":
        log_warning("No port selected")
        return

    dpg.configure_item("reset_button", enabled=True)
    dpg.configure_item("scan_button", enabled=True)
    dpg.configure_item("port_close_button", enabled=True)
    log_info("Reset button enabled")
    log_info("Scan button enabled")
    log_info("Port close button enabled")

    log_info(f"Opening serial connection to port: {selected_port}")

    serial_port = serial.Serial(selected_port, 115200, timeout=1)

    if serial_port.is_open:
        log_info(f"Serial port opened: {selected_port}")
    else:
        log_error(f"Failed to open serial port: {selected_port}")

def on_locking_click(sender):
    global lock, serial_port

    if not (serial_port is not None and serial_port.is_open):
        log_info("Serial port already opened")
        return

    lock = not lock

    # lock will be False after Locked, True after Unlocked
    if lock:
        dpg.configure_item(sender, label="Unlocked")
        serial_port.write(b'u')
        log_debug("Unlocked")
    else:
        dpg.configure_item(sender, label="Locked")
        serial_port.write(b'l')
        log_debug("Locked")


def update_ports_combo():
    esp32_ports = find_esp32_ports()
    port_names = list(esp32_ports.items())
    dpg.configure_item("port_selection", items=port_names)

def update_button_status():
    global winner_id
    # Check if serial port exist and is open
    if not (serial_port is not None and serial_port.is_open):
        return

    if serial_port.in_waiting == 0:
        return

    esp32_data = serial_port.readline().decode('utf-8').strip("\n\r")

    # Check if the data is regarding the winner
    if "WSSID" in esp32_data:
        winner_ssid = esp32_data.split(";")
        log_debug(f"Winner SSID: {winner_ssid[1]}")
        for i in buttons_info:
            if buttons_info[i]['SSID'] == winner_ssid[1]:
                winner_id = i
                log_debug(f"Button {i} is the winner")
                play_video(f"winner{i}")
                dpg.configure_item(f'button_shape_{i}', color=winner_bg_color, fill=winner_bg_color)
                return
        log_debug(f"Winner SSID: {winner_ssid[1]} not found in the list of buttons")


def ping_button_location(button_id):
    if not (serial_port is not None and serial_port.is_open):
        return

    serial_port.write(bytes(f"{str(button_id)}", 'utf-8'))

def on_ping_button_clicked(sender):
    global winner_id
    id = int(sender[-1])

    if dpg.is_item_hovered(sender):
        if id > len(buttons_info):
            log_warning(f"Button {id} does not exist")
            return
        if serial_port is not None and serial_port.is_open:
            ping_button_location(id)
            log_info(f"Pinging button {id}")
        else:
            log_warning(f"Serial port not open, unable to ping button {id}")

def on_reset_click():
    global winner_id, lock
    if not (serial_port is not None and serial_port.is_open):
        return

    serial_port.write(b'r')
    winner_id = 0
    log_info("Resetting game")
    for i in range(1, len(buttons_info)+1):
        if buttons_info[i]['SSID'] != "None":
            log_debug(f"Resetting button {i}")
            dpg.configure_item(f'button_shape_{i}', color=online_bg_color, fill=online_bg_color)
            dpg.configure_item("Lock/Unlock", label="Locked")
            lock  = False
            serial_port.write(b'l')

# def on_scan_click(sender, app_data, user_data):
def on_scan_click():
    if not (serial_port is not None and serial_port.is_open):
        return

    serial_port.write(b's')
    log_info("Scanning to update buttons data")

    scanning_buttons()

def process_score_input():
    points = dpg.get_value("Input Field")

    points_only_pattern = r'^-?\d+$'
    points_group_pattern = r'^\d+,-?\d+$'

    if re.match(points_only_pattern, points):
        if dpg.is_item_hovered("Minus"):
            points = -int(points)
        if 1 <= int(winner_id) <= 5:
            log_info(f"Group {winner_id} is getting {points} points!")
            update_score(winner_id, points)
        else:
            log_info(f"Invalid winner id {winner_id}! Please try again")
    elif re.match(points_group_pattern, points):
        group, points = points.split(",")
        if dpg.is_item_hovered("Minus"):
            points = -int(points)
        if 1 <= int(group) <= 5:
            log_info(f"Group {group} is getting {points} points!")
            update_score(group, points)
        else:
            log_info(f"Invalid group {group}! Please try again")
    else:
        log_info(f"Invalid input! Please try again")

def scanning_buttons():
    if not (serial_port is not None and serial_port.is_open):
        return

    esp32_data = serial_port.readline().decode('utf-8').strip("\n\r")

    if esp32_data == "":
        log_info("No buttons found")
        return

    log_debug(f"Button data received from controller: {esp32_data}")

    idx = 0
    # Check if the data is regarding the button
    # Data format: ID;1|SSID;30:AE:A4:1A:2B:3C|RSSI;-50|BAT;100|STA;1
    if "ID" in esp32_data:
        s = esp32_data.split("|")
        for ele in s:
            key = (ele.split(";"))[0] or ""
            value = (ele.split(";"))[1] or ""
            # Create a dictionary to store the button information
            # e.g. {1: {'SSID': '30:AE:A4:1A:2B:3C ', 'RSSI': '-50', 'BAT': '100', 'STA': '1'}}
            # TODO: Implement a way to check if data format is strictly adhered (data corruption check)
            if key == "" or value == "":
                continue
            if key == "ID":
                idx = int(value) + 1
                buttons_info[idx] = {}
            else:
                buttons_info[idx][key] = value

    for i in buttons_info:
        if buttons_info[i]['STA'] != "0":
            log_debug(f"Button {i} SSID: {buttons_info[i]['SSID']}")
            dpg.configure_item(f'button_ssid_{i}', text=f"SSID:{buttons_info[i]['SSID']}")
            dpg.configure_item(f'button_shape_{i}', color=online_bg_color, fill=online_bg_color)
            dpg.configure_item(f'button_status_{i}', text=f"Status: Online")
            dpg.configure_item(f'button_rssi_{i}', text=f"RSSI: {buttons_info[i]['RSSI']}")
            dpg.configure_item(f'button_battery_{i}', text=f"Battery: {buttons_info[i]['BAT']}")
        else:
            buttons_info[i]['SSID'] = "None"
            log_debug(f"Button {i} is offline")
            dpg.configure_item(f'button_shape_{i}', color=offline_bg_color, fill=offline_bg_color)
            dpg.configure_item(f'button_status_{i}', text=f"Status: Offline")
            dpg.configure_item(f'button_ssid_{i}', text=f"SSID: None")
            dpg.configure_item(f'button_rssi_{i}', text=f"RSSI: None")
            dpg.configure_item(f'button_battery_{i}', text=f"Battery: None")

    if serial_port.in_waiting > 0:
        scanning_buttons()

def on_close_click():
    global serial_port
    if serial_port is not None and serial_port.is_open:
        serial_port.close()
        log_info("Serial port closed")
    dpg.configure_item("reset_button", enabled=False)
    dpg.configure_item("scan_button", enabled=False)
    dpg.configure_item("port_close_button", enabled=False)
    log_info("Reset button disabled")
    log_info("Scan button disabled")
    log_info("Port close button disabled")

def main(args):
    global logx, serial_port, mp4_config

    log_level = args.log_level

    text_color = (255, 255, 255)
    text_size = 15

    dpg.create_context()
    dpg.create_viewport(title='MT24 Buzzer', width=1280, height=1080, x_pos=0, y_pos=0)
    dpg.setup_dearpygui()

    # with dpg.window(label="MT24 Buzzer", width=1250, height=750):
    with dpg.window(label="COM", no_close=True, width=500, height=130, pos=(0, 0)):
        dpg.add_text("Select the COM port of the ESP32 device:")
        with dpg.group(horizontal=True):
            dpg.add_combo(label="Ports", tag='port_selection', items=[], callback=on_port_select)
            dpg.add_button(label="Select", tag='port_select_button', callback=on_port_select_button_click)
            dpg.add_button(label="Close", tag='port_close_button', callback=on_close_click, enabled=False)

        with dpg.group(horizontal=True):
            dpg.add_button(label="RESET", tag='reset_button', callback=on_reset_click, enabled=False, width=100, height=50)
            dpg.add_button(label="SCAN", tag='scan_button', callback=on_scan_click, enabled=False, width=100, height=50)


    with dpg.window(label="Buzzer Status", tag="buzzer_list", pos=(0, 150), width=1280, height=250, no_close=True):
        with dpg.group(horizontal=True, horizontal_spacing=10):
            for i in range(1,6):
                with dpg.drawlist(width=240, height=200, pos=(200*(i), 0), tag=f'button_drawlist_{i}', callback=on_ping_button_clicked):
                    x, y = 10, 20
                    dpg.draw_rectangle((0, 0), (240, 200), tag=f'button_shape_{i}', color=offline_bg_color, fill=offline_bg_color)
                    dpg.draw_text((50, y), f"Button {i}", color=text_color, size=text_size, tag=f'button_text_{i}')
                    dpg.draw_text((x, y+30), "Status: Offline", color=text_color, size=text_size, tag=f'button_status_{i}')
                    dpg.draw_text((x, y+60), "SSID: None", color=text_color, size=text_size, tag=f'button_ssid_{i}')
                    dpg.draw_text((x, y+90), "RSSI: None", color=text_color, size=text_size, tag=f'button_rssi_{i}')
                    dpg.draw_text((x, y+120), "Battery: None", color=text_color, size=text_size, tag=f'button_battery_{i}')

    with dpg.window(label="Score Input", width=800, height=100, pos=(0, 400), no_close=True):
        dpg.add_text("Input the points to be added or deducted")
        with dpg.group(horizontal=True):
            dpg.add_input_text(tag="Input Field", width=400, on_enter=True, hint="Group,Points or Points only", callback=process_score_input)
            dpg.add_button(label="+", tag="Plus", height=40, width=100, callback=process_score_input)
            dpg.add_button(label="-", tag="Minus", height=40, width=100, callback=process_score_input)

    with dpg.window(label="Locking Mechanism", width=300, height=100, pos=(850, 400), no_close=True):
        dpg.add_button(label="Locked", tag="Lock/Unlock", height=60, width=300, callback=on_locking_click)

    with dpg.window(label="Logging", tag="log", pos=(0, 500), width=1280, height=300, no_close=True):
        logx = mvLogger(parent="log")
        logx.log_level = log_level

    dpg.show_viewport()


    # below replaces, start_dearpygui()
    while dpg.is_dearpygui_running():
        # insert here any code you would like to run in the render loop
        update_ports_combo()
        if lock:
            # log_debug(f"Button state {lock}")
            update_button_status()
        # scanning_buttons()
        dpg.render_dearpygui_frame()

    dpg.destroy_context()


if __name__ == "__main__":
    args = argparse.ArgumentParser(description="MT24 Buzzer GUI")
    args.add_argument("--log_level", type=int, default=2, help="Log level")
    args = args.parse_args()
    main(args)
