# MiniNav for the Waveshare ESP32-C6-Touch-AMOLED-2.16

Turn-by-turn display for a 480x480 AMOLED with **no PSRAM**. The phone page (`docs/app`) plans the
route (OpenStreetMap) and sends the next manoeuvre over Bluetooth LE (Web Bluetooth); the firmware
draws arrow, distance, street, speed and ETA. It does not mirror Android Auto: that needs far more
RAM than this chip has.

Display, touch, PMIC and pin setup come from Waveshare's own ESP-IDF demo for this board
(`components/`), so the hardware bring-up is the known-good one.

## Get the installer website (no local toolchain needed)

1. Create a new GitHub repository and upload everything in this folder (keep the `.github` folder).
2. Repo **Settings > Pages > Build and deployment > Source: GitHub Actions**.
3. Open the **Actions** tab and run "Build firmware and publish the installer page" (or push to `main`).
4. When it's green, your site is at `https://<your-user>.github.io/<repo>/`. Open it in Chrome on a computer and click Install.

## Build locally instead

ESP-IDF v5.5.2, then: `idf.py set-target esp32c6 && idf.py build`, and for the web image:
`cd build && python -m esptool --chip esp32c6 merge_bin -o ../docs/flash/MiniNav-C6.bin --flash_mode dio --flash_freq 80m --flash_size 16MB @flash_args`

## Serial protocol (phone -> board, text lines)

    N,<kind>,<angle>,<dist_m>,<speed_kmh>,<remain_m>,<eta_min>,<street>   kind 0 turn, 1 arrived
    B,<0-100>   brightness      X   back to the idle screen

BLE service `4d696e69-4e61-7600-0000-000000000001`, write characteristic `...0002`.
