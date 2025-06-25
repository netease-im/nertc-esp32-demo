# 如何搭建和运行云信 ESP32 Demo 工程

## 准备工作

- ESP IDF 开发套件
- ESP32 的开发板
- 云信嵌入式 SDK

## 环境安装

### 安装 ESP IDF 开发套件

#### Windows

推荐采用离线下载和安装 ESP IDF 开发套件：https://dl.espressif.com/dl/esp-idf/ ，选择对应版本的安装文件进行下载即可。不建议采用 VSCODE 插件的方式进行安装，后续使用过程中比较容易出错。

##### 安装步骤

推荐下载最新版本（包含摄像头开发套件）的离线安装包，下载完成之后双击 EXE 文件开始安装。

安装结束时，如果勾选了 `Run ESP-IDF PowerShell Environment` 或 `Run ESP-IDF Command Prompt (cmd.exe)`，安装程序会在选定的提示符窗口启动 ESP-IDF。

##### 使用命令提示符

ESP-IDF 工具安装器可在"开始"菜单中，创建一个打开 ESP-IDF 命令提示符窗口的快捷方式。本快捷方式可以打开 Windows 命令提示符（即 cmd.exe），并运行 export.bat 脚本以设置各环境变量（比如 PATH，IDF_PATH 等）。此外，还可以通过 Windows 命令提示符使用各种已经安装的工具。

**注意**，本快捷方式仅适用 ESP-IDF 工具安装器中指定的 ESP-IDF 路径。如果电脑上存在多个 ESP-IDF 路径（比如需要不同版本的 ESP-IDF），有以下两种解决方法：

1. 为 ESP-IDF 工具安装器创建的快捷方式创建一个副本，并将新快捷方式的 ESP-IDF 工作路径指定为希望使用的 ESP-IDF 路径。
2. 或者，可以运行 cmd.exe，并切换至希望使用的 ESP-IDF 目录，然后运行 export.bat。注意，这种方法要求 PATH 中存在 Python 和 Git。如果在使用时遇到有关"找不到 Python 或 Git"的错误信息，请使用第一种方法。

##### 验证安装成功

在 ESP-IDF PowerShell 窗口中执行以下命令，输出正确的 ESP-IDF 版本即安装成功。

```bash
idf.py --version
```

##### 连接设备

请将 ESP32 开发板连接到 PC，并查看开发板使用的串口。在 Windows 操作系统中，串口名称通常以 COM 开头。

有关如何查看串口名称的详细信息，请见 [与 ESP32 创建串口连接](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/establish-serial-connection.html)。

#### MacOS

Catalina 10.15 发布说明中表示不推荐使用 Python 2.7 版本，因此推荐安装 Python 3，具体的安装方式参考网上资料。

##### 安装步骤

推荐直接去下载最新版本（包含摄像头开发套件）的离线包：https://github.com/espressif/esp-idf 或者采用 git 拉取的方式获取

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.4.1 --recursive https://github.com/espressif/esp-idf.git
```

安装 ESP-IDF 使用的各种工具，比如编译器、调试器、Python 包等

```bash
# 仅为 ESP32 安装所需工具
cd ~/esp/esp-idf
./install.sh esp32

# 仅为 ESP32 的特定芯片安装所需工具，如 esp32c3 和 esp32s3
cd ~/esp/esp-idf
./install.sh esp32c3,esp32s3

# 一次性为所有支持的目标芯片安装工具
cd ~/esp/esp-idf
./install.sh all
```

对于 macOS 用户，如在上述任何步骤中遇到以下错误:

```
<urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: unable to get local issuer certificate (_ssl.c:xxx)
```

可运行电脑 Python 文件夹中的 `Install Certificates.command` 安装证书。了解更多信息，请参考 [安装 ESP-IDF 工具时出现的下载错误](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html#installation-troubleshooting)。

##### 设置环境变量

上述步骤安装的工具尚未添加至 PATH 环境变量，无法通过"命令窗口"使用这些工具。因此，必须设置一些环境变量。这可以通过 ESP-IDF 提供的另一个脚本进行设置。

请在需要运行 ESP-IDF 的终端窗口运行以下命令：

```bash
. $HOME/esp/esp-idf/export.sh
```

如果需要经常运行 ESP-IDF，可以为执行 export.sh 创建一个别名，具体步骤如下：

A. 复制并粘贴以下命令到 shell 配置文件中（.profile、.bashrc、.zprofile 等）

```bash
alias get_idf='. $HOME/esp/esp-idf/export.sh'
```

B. 通过重启终端窗口或运行 `source [path to profile]`，如 `source ~/.bashrc` 来刷新配置文件。

现在可以在任何终端窗口中运行 `get_idf` 来设置或刷新 ESP-IDF 环境。

**注意**：不建议直接将 export.sh 添加到 shell 的配置文件。这样做会导致在每个终端会话中都激活 IDF 虚拟环境（包括无需使用 ESP-IDF 的会话）。这违背了使用虚拟环境的目的，还可能影响其他软件的使用。

##### 验证安装成功

在设置好环境变量的"命令窗口"执行以下命令，输出正确的 ESP-IDF 版本即安装成功。

```bash
idf.py --version
```

##### 连接设备

请将 ESP32 开发板连接到 PC，并查看开发板使用的串口。通常，串口在 macOS 操作系统的名称通常以 `/dev/cu.` 开头。有关如何查看串口名称的详细信息，请见 [与 ESP32 创建串口连接](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/establish-serial-connection.html)。

## 运行 Demo

### 源码下载

下载 Demo 工程源码：https://github.com/netease-im/nertc-esp32-demo ，直接下载 zip 压缩包或者通过 git clone 的方式获取。

### 工程配置

#### ESP32C3

在设置好环境变量的"命令窗口"进入 Demo 工程目录，首先使用 `idf.py set-target esp32` 设置"目标"芯片。注意，此操作将清除并初始化项目之前的编译和配置（如有）。也可以直接将"目标"配置为环境变量（此时可跳过该步骤）。更多信息，请见 [选择目标芯片：set-target](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/build-system.html#set-target)。

```bash
cd nertc-esp32-demo
# idf.py --list-targets # 可选，列举出支持的target
idf.py set-target esp32c3
```

运行工程配置工具 menuconfig

```bash
idf.py menuconfig
```

在配置页面上选择 "Xiaozhi Assistant --->"，然后进行如下设置：

- 板子类型选择当前连接的 c3 板子，例如：`Board Type (虾哥 Mini C3) --->`
- 开启 NERTC Protocol，即：`[*] Enanle NERTC Protocol`
- 输入对应的 APP KEY，即：`(YOU_APP_KEY) NERTC Appkey`
- 开启 NERTC 的音频云端 AEC 功能（如果需要），即：`[*] Enable NERTC Server-Side AEC`

#### ESP32S3

在设置好环境变量的"命令窗口"进入 Demo 工程目录，首先使用 `idf.py set-target esp32` 设置"目标"芯片。注意，此操作将清除并初始化项目之前的编译和配置（如有）。也可以直接将"目标"配置为环境变量（此时可跳过该步骤）。更多信息，请见 [选择目标芯片：set-target](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-guides/build-system.html#set-target)。

```bash
cd nertc-esp32-demo
# idf.py --list-targets # 可选，列举出支持的target
idf.py set-target esp32s3
```

运行工程配置工具 menuconfig

```bash
idf.py menuconfig
```

在配置页面上选择 "Xiaozhi Assistant --->" 之后，进行如下设置：

- 板子类型选择当前连接的 s3 板子，例如：`Board Type (面包板新版接线（WiFi）) --->`
- 开启 NERTC Protocol，即：`[*] Enanle NERTC Protocol`
- 输入对应的 APP KEY，即：`(YOU_APP_KEY) NERTC Appkey`
- 开启 NERTC 的音频云端 AEC 功能（如果需要），即：`[*] Enable NERTC Server-Side AEC`
- **注意**，如果开启云端 AEC 功能，必须要关闭 Audio Noise Reduction，即：`[ ] Enable Audio Noise Reduction`
- 开启 NERTC 网络 Qos 策略（建议），即：`[*] Enable NERTC QOS`
- 开启唤醒词检测（如果板子支持，建议开启），即：`[*] Enable Wake Word Detection (AFE)`

### 编译 && 启动 Demo

使用 build 命令编译或者直接使用 flash 命名进行烧录：

```bash
# 仅编译，将生成 .bin 文件
idf.py build

# 编译 + 烧录
idf.py -p PORT flash

# 编译 + 烧录 + 监视输出
idf.py -p PORT flash monitor
```

### 首次启动

Demo 首次启动之后，会提示对设备进行联网操作。如果之前设备从未注册和绑定过，需要先登录到云信后台，然后用此时设备上显示的 Device ID（即设备显示的热点名称）进行注册和绑定。完成之后，就可以按照设备的提示音指引，完成联网操作。

### 运行成功

设备联网成功之后，Demo 会开启连接 NERTC 服务的流程。如果监视输出的话，可以看到如所示的打印输出，说明 Demo 运行成功。
```bash
I (8750) MCP: Add tool: self.audio_speaker.set_volume
  (8760) [main] Engine: NERtcEngine
  (8760) [main] Engine: Initialize
  (8760) [main] Engine: NERtcThread
  (8770) [main] Engine: NERtcThread::Start
I (8770) NeRtcProtocol: create NeRtcProtocol success device_id:yunxin-0DC9
I (8780) NeRtcProtocol: cname = 79452792

  (8780) [main] Engine: post JoinChannel
  (8790) [rtc_worker] Engine: JoinChannel, has token: 0
  (8790) [rtc_worker] Engine: new_state: 1, msg: join channel
I (8800) NeRtcProtocol: NERtc OnChannelStatusChanged: 1, join channel
  (8810) [rtc_worker] Signal: [GetChannelInfo]
  (8810) [rtc_worker] HTTPClient: Opening HTTP connection to https://nrtc.netease.im/nrtc/getChannelInfos.action
I (8820) MODEL_LOADER: The storage free size is 22400 KB
I (8830) MODEL_LOADER: The partition size is 960 KB
I (8830) MODEL_LOADER: Successfully load srmodels
I (8840) AfeWakeWord: Model 0: wn9_nihaoxiaozhi_tts
I (8840) AFE_CONFIG: Set WakeNet Model: wn9_nihaoxiaozhi_tts
MC Quantized wakenet9: wakenet9l_tts1h8_你好小智_3_0.631_0.635, tigger:v4, mode:0, p:0, (Jun 16 2025 16:18:15)
I (8880) AFE: AFE Version: (1MIC_V250121)
I (8890) AFE: Input PCM Config: total 1 channels(1 microphone, 0 playback), sample rate:16000
I (8890) AFE: AFE Pipeline: [input] ->  -> |VAD(WebRTC)| -> |WakeNet(wn9_nihaoxiaozhi_tts,)| -> [output]
I (8900) AfeWakeWord: Audio detection task started, feed size: 512 fetch size: 512
I (8910) Application: STATE: idle
I (8910) SystemInfo: free sram: 94279 minimal sram: 92843
I (8920) Application: Resampling audio from 16000 to 24000
I (8920) OpusResampler: Resampler configured with input sample rate 16000 and output sample rate 24000
I (9050) esp-x509-crt-bundle: Certificate validated
  (10070) [rtc_worker] HTTPClient: request successful, status_code: 200 content_length:ld
  (10080) [rtc_worker] Signal: get channel info success, cid:1349104324759499 ws_addr:115.236.119.104:6999 token:cc822677e372478385553458a235e14a
  (10090) [rtc_worker] Signal: [Connect]
  (10090) [rtc_worker] WSClient: Connecting to ws://115.236.119.104:6999/?cid=1349104324759499&uid=6669&deviceId=yunxin-0DC9
  (10300) [rtc_worker] Signal: Signal Connect
  (10300) [rtc_worker] Engine: OnSignalConnected
  (10300) [rtc_worker] Signal: Login
  (10480) [rtc_worker] Signal: recv OnLogin
  (10480) [rtc_worker] Engine: OnLoginAck, code:200 msg:
  (10480) [rtc_worker] Engine: local ssrc: 112691457
  (10490) [rtc_worker] Engine: StartHBTimer
  (10490) [rtc_worker] Signal: SendHeartbeat
  (10500) [rtc_worker] MediaTrans: PrepareConnection: 115.236.119.104:4429-1
  (10510) [rtc_worker] MediaTrans: start connected: 0x3c28dd8c
  (10510) [rtc_worker] Engine: prepare connection addr = 115.236.119.104:4429
  (10520) [rtc_worker] Engine: new_state: 2, msg: join channel success
I (10530) NeRtcProtocol: NERtc OnChannelStatusChanged: 2, join channel success
I (10530) NeRtcProtocol: NERtc OnJoin: cid:1349104324759499, uid:6669, code:0, elapsed:0 sample_rate:8000 samples_per_channel:80
```

如果控制台有类似 "NeRtcProtocol: NERtc OnChannelStatusChanged: 2, join channel success" 的输出或者听到板子发出 2 次提示音，说明 Demo 已经成功加入 NERTC 房间，这个时候可以根据板子的实际情况选择"按键触发" 或 "唤醒词触发"的方式开启 AI 对话了。