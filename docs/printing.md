# Bluetooth Printing

Desktop-D30 connects directly to your Phomemo D30 printer using Bluetooth Classic.

## Connection Setup

### On Windows
1. Ensure your D30 printer is turned on.
2. Open the **Printer** menu in Desktop-D30.
3. Select **Scan for Devices**.
4. Find your printer in the list (usually appears as `D30_XXXX`) and click **Connect**.
5. Once the status indicator turns green, you are ready to print.

!!! note "Driver Note"
    On Windows, you do not need to install any special drivers. Desktop-D30 uses the standard Windows Bluetooth stack.

### On Linux
1. Ensure your D30 printer is turned on.
2. Go to the **Printer** menu and select **Scan for Devices**.
3. Choose your printer and click **Connect**.

!!! tip "Bluetooth Permissions"
    If the application cannot find your Bluetooth adapter on Linux, ensure your user is in the `lp` or `bluetooth` group:
    `sudo usermod -aG bluetooth $USER` (reboot required after running).

## Printing Labels

Once connected, you can print the current canvas by going to **Printer -> Print Single Label**.

For printing multiple labels from a data source, see the [Batch Printing](batch-printing.md) guide.
