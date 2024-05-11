import zmq
import time
import random
import threading
import curses
import os
import sys

# Get the absolute path to the directory containing this script
current_script_directory = os.path.dirname(os.path.abspath(__file__))
# Get the absolute path to the project folder
project_folder_path = os.path.abspath(os.path.join(current_script_directory, ".."))
# Add the project folder to the Python path
sys.path.append(project_folder_path)

from src import client_server_pb2 as proto
from src.client_server import *

context = zmq.Context()
requester = context.socket(zmq.REQ)

stdscr = None

stop = False


def close_program(message: str = "\nInternal error.", flag: int = 1):
    global stop
    stop = True
    curses.nocbreak()
    stdscr.keypad(False)
    curses.echo()
    curses.curs_set(1)
    curses.endwin()
    requester.close()
    context.term()
    print(message)
    sys.exit(flag)


def thread_getch():
    global stop
    key = None
    while key != ord("q") and not stop:
        try:
            key = stdscr.getch()  # Non-blocking getch
        except KeyboardInterrupt:
            break

    stop = True


def thread_control_wasps():
    global stop

    # Generate a random number of wasps
    num_wasps = random.randint(1, 10)
    try:
        zmq_send_WaspConnectReq(requester, num_wasps)
    except Exception as e:
        close_program()

    # Receive and process connection response
    try:
        resp = zmq_recv_BotConnectResp(requester)
    except Exception as e:
        close_program()

    client_id = resp.client_id
    if client_id == -1:
        close_program("\nSorry! Server is full.", 0)

    token = resp.token
    wasps_ids = resp.bot_id
    num_wasps = len(wasps_ids)

    counter = 0
    timeout = False
    while not stop:
        time.sleep(random.uniform(0, 0.7))
        index = random.randint(0, num_wasps - 1)
        direction = random.randint(0, 3)

        # Send movement message
        try:
            zmq_send_WaspMovementReq(
                requester, wasps_ids[index], token, direction, client_id
            )
        except Exception as e:
            close_program()

        # Receive answer from server
        try:
            resp = zmq_recv_BotMovementResp(requester)
        except Exception as e:
            close_program()

        if resp.resp == proto.SUCCESS:
            match direction:
                case proto.LEFT:
                    move = "to the left"
                case proto.RIGHT:
                    move = "to the right"
                case proto.UP:
                    move = "up"
                case proto.DOWN:
                    move = "down"
                case _:
                    move = ""
            stdscr.move(index, 0)
            stdscr.clrtoeol()  # Clear from the current position to the end of the line
            stdscr.addstr(index, 0, f"{counter} Wasp {index}: moved {move}")
            stdscr.refresh()
            counter += 1

        elif resp.resp == proto.TIMEOUT:
            timeout = True
            break

    if not timeout:
        # Send disconnect message
        try:
            zmq_send_WaspDisconnectReq(requester, client_id, token)
        except Exception as e:
            close_program()

        # Receive disconnect response
        try:
            resp = zmq_recv_BotDisconnectResp(requester)
        except Exception as e:
            close_program()
        # Do something with the response ?


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <address:port>")
        print(f"Example: {sys.argv[0]} 127.0.0.1:5557")
        sys.exit(1)

    endpoint = f"tcp://{sys.argv[1]}"
    requester.connect(endpoint)

    print("Connected!")

    global stdscr
    stdscr = curses.initscr()
    curses.cbreak()
    stdscr.keypad(True)
    curses.noecho()
    curses.curs_set(0)
    # Set non-blocking input
    stdscr.nodelay(True)
    stdscr.refresh()

    worker_threads = []
    worker_threads.append(threading.Thread(target=thread_control_wasps))
    worker_threads.append(threading.Thread(target=thread_getch))

    for thread in worker_threads:
        thread.start()

    for thread in worker_threads:
        thread.join()

    close_program("Disconnected!", 0)


if __name__ == "__main__":
    main()
