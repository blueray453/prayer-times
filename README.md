# Prayer Times Dashboard

A modern, open-source desktop application to display daily Islamic prayer times, the current prayer, and prohibited times, built with **Qt6** and **C++17**. The application fetches data from the IslamicAPI service and caches it locally for **24 hours** to minimize network requests.

> [!WARNING]
> ## ⚠️ Disclaimer
>
> This application is **not an official source of prayer times** and **should not be relied upon as the sole authority for religious observance**.
>
> I am **not an Islamic scholar or a specialist in prayer time calculations**. This project simply retrieves prayer times from the IslamicAPI service and displays the data it receives. I do **not** calculate or verify the prayer times myself.
>
> Although the application attempts to present the information accurately, errors may occur due to API data, calculation methods, location settings, configuration mistakes, software bugs, network issues, or other unforeseen factors.
>
> **Always verify prayer times with your local mosque or other trusted official sources before relying on this application for religious purposes.** By using this software, you acknowledge that you are responsible for confirming the accuracy of the information for your own location and circumstances.

> 📷 **Screenshot**
>
> Add a screenshot here later:
>
> ![Prayer Times Dashboard](https://screenshot.png)

---

## ✨ Features

- 🕒 Real-time clock with live prayer time updates.
- 🕌 **Current prayer highlighting** – automatically highlights the active prayer period.
- 📅 Hijri and Gregorian dates displayed side by side.
- 🌅 Clearly displays prohibited prayer times (e.g., sunrise and midday).
- ⚙️ Built-in settings dialog to configure:
  - API key
  - Location
  - Prayer calculation method
- 💾 Offline caching for 24 hours to reduce network usage.
- 🔒 Open-source friendly – no secrets are stored in the source code. All user configuration is kept in `config.json`, which is excluded from Git.

---

## 📋 Prerequisites

You'll need:

- Qt6 (Core, Widgets, Network)
- CMake **3.16+**
- A C++17 compatible compiler (GCC, Clang, or MSVC)
- An API key from **IslamicAPI** (free tier available)

### Ubuntu / Debian

```bash
sudo apt install qt6-base-dev qt6-base-dev-tools qt6-network-dev
```

For other operating systems, follow the official Qt installation guide.

---

## 🔧 Building

Clone the repository and use the provided build script:

```bash
git clone https://github.com/yourusername/prayer-times-dashboard.git
cd prayer-times-dashboard
./run.sh
```

The script will:

- Create the `build/` directory (if necessary).
- Configure the project using CMake.
- Build using all available CPU cores.
- Copy a default `config.json` into the build directory if one doesn't already exist.
- Launch the application.

### Manual Build

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
./prayer_times
```

---

## 🧹 Cleaning

To remove the build directory and start from scratch:

```bash
./run.sh --clean
```

---

## ⚙️ Configuration

All configuration is stored in `config.json` located in the same directory as the executable (typically `build/`).

The file is intentionally ignored by Git.

On first launch, the application automatically creates a default configuration with empty values.

You can configure the application in two ways.

### 1. GUI Settings

1. Click the **Settings** button in the top-right corner.
2. Enter:
   - API key
   - Latitude
   - Longitude
   - Calculation method
3. Click **OK**.

The application automatically saves the configuration and downloads fresh prayer times.

---

### 2. Edit `config.json` Manually

```json
{
  "api_key": "YOUR_API_KEY",
  "location": {
    "lat": 23.8365,
    "lon": 90.3695,
    "name": "Dhaka"
  },
  "method": 4
}
```

### Configuration Fields

| Field | Description |
|------|-------------|
| `api_key` | Your IslamicAPI API key |
| `location.lat` | Latitude (decimal degrees) |
| `location.lon` | Longitude (decimal degrees) |
| `location.name` | Display name of the location |
| `method` | Prayer calculation method ID |

---

## 📖 Supported Calculation Methods

| ID | Method |
|----|--------|
| 1 | University of Islamic Sciences, Karachi |
| 3 | Muslim World League (MWL) |
| 4 | Umm al-Qura University, Makkah *(Default)* |
| 5 | Egyptian General Authority of Survey |

---

## 🚀 Usage

After building and configuring the application, run:

```bash
./build/prayer_times
```

The interface includes:

- **Top bar**
  - Timezone
  - Hijri date
  - Gregorian date
  - Live clock

- **Left panel**
  - Daily prayer times
  - Current prayer highlighted in blue

- **Right panel**
  - Prohibited prayer time intervals

The application:

- Updates the current prayer every second.
- Automatically refreshes prayer data when the local cache is older than 24 hours.

---

## 🗂️ Project Structure

```text
.
├── CMakeLists.txt          # CMake build configuration
├── config.json             # User configuration (ignored by Git)
├── main.cpp                # Application source code
├── run.sh                  # Build & run helper script
├── .gitignore              # Ignores build/, config.json, etc.
└── README.md               # Project documentation
```

---

## 🤝 Contributing

Contributions are welcome!

If you'd like to improve the project:

1. Fork the repository.
2. Create a feature branch.
3. Make your changes.
4. Test your changes.
5. Submit a pull request.

Please follow the existing coding style and ensure new functionality is tested before submitting.

---
