import asyncio
from bleak import *
from datetime import datetime
import signal
from supabase import create_client, Client

# Supabase configuration
SUPABASE_URL = "https://rxckwzfakcepeoqmczqo.supabase.co"
SUPABASE_KEY = "XXXXX"

supabase: Client = create_client(SUPABASE_URL, SUPABASE_KEY)

HEART_RATE_UUID = '00002a37-0000-1000-8000-00805f9b34fb'
DEVICE_ADDRESS = 'A40253A0-F125-EACC-4AEC-92DE3FBE56D3'

async def run_heart_rate_monitor(address, interval=0.3):
    print(f"Connecting to device at {address}...")
    async with BleakClient(address, timeout=60) as client:
        if await client.is_connected():
            print(f"Connected to device at {address}")
            running = True

            def handle_stop_signals(signum, frame):
                nonlocal running
                print("\nStopping heart rate monitor...")
                running = False

            # Register signal handlers for graceful shutdown
            signal.signal(signal.SIGINT, handle_stop_signals)
            signal.signal(signal.SIGTERM, handle_stop_signals)

            def callback(sender, data):
                if running:
                    try:
                        heart_rate = int.from_bytes(data[1:2], byteorder='little')
                        now = datetime.now()
                        formatted_time = now.strftime("%H:%M:%S")
                        print(f"Timestamp: {formatted_time}, Heart Rate: {heart_rate} BPM")

                        # Insert data into Supabase
                        data = {
                            'timestamp': now.strftime("%Y-%m-%d %H:%M:%S"),
                            'heart_rate': heart_rate
                        }
                        supabase.table('heart_rate_data').insert(data).execute()

                    except Exception as e:
                        print(f"An error occurred: {str(e)}")

            await client.start_notify(HEART_RATE_UUID, callback)
            print("Started receiving heart rate data...")

            # Loop until stopped by signal
            while running:
                await asyncio.sleep(interval)

            await client.stop_notify(HEART_RATE_UUID)
        else:
            print("Failed to connect.")

if __name__ == "__main__":
    try:
        asyncio.run(run_heart_rate_monitor(DEVICE_ADDRESS))
    except KeyboardInterrupt:
        print("\nScript interrupted by user.")