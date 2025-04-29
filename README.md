# 🐚 ReShell - XOR Encrypted Reverse Shell Toolkit

---

## 📝 Description

**ReShell** is a lightweight XOR-encrypted reverse shell and cross-platform listener toolkit designed for educational purposes and controlled environments such as penetration testing labs or home network research.

---

## 💻 Usage

1. Open the ReShell project.
2. Modify the hardcoded IP and port in the source to match your listener.
3. Compile and run on the target machine.

---

## 📡 Starting a Listener 
#### ▶️ Windows C++ Listener
- `ListenerWin.exe`
#### ▶️ Linux C++ Listener
- `./listener_linux`
#### ▶️ Python Listener (Cross-Platform)
- `python3 ListenerPython.py`

This version works on any platform with Python 3 installed. It will automatically decrypt incoming XOR-encrypted data.

---

## 🛠️ Customization
#### Encryption Key
- Change the XOR_KEY constant in both ReShell and each listener to ensure encrypted traffic is correctly decrypted.

#### Target IP & Port
- Modify these in ReShell to set the callback destination.

#### Console Output
- All listeners print decrypted shell output and allow for typed input.

---

## ✅ Features
- XOR-encrypted reverse shell
- C++ listeners for Windows and Linux
- Python listener for maximum portability
- Works over LAN/WAN (with port forwarding)
- Simple to integrate, extend, or obfuscate further

---

