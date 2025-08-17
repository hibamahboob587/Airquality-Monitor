from django.shortcuts import render
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.utils import timezone
from .models import SensorData
import json
from datetime import timedelta
from .forms import FirmwareUploadForm
import os
from django.http import HttpResponseRedirect

def success_page(request):
    
    message = "Firmware uploaded successfully!"
    return render(request, 'success.html', {'message': message})

def dashboard(request):
    
    current_data = SensorData.objects.latest('timestamp')
    return render(request, 'dashboard.html', {'current_data': current_data})


def get_all_data(request):
    
    
    try:
        
        all_data = SensorData.objects.all().order_by('-timestamp')[:50]
        data_points = []

        for entry in reversed(all_data):  
            data_points.append({
                'timestamp': entry.timestamp.strftime('%H:%M:%S'),
                'temperature': entry.temperature,
                'humidity': entry.humidity,
                'airQuality': entry.airQuality,   
            })

        return JsonResponse(data_points, safe=False)

    except Exception as e:
        print(f"[ERROR] get_all_data: {e}")
        return JsonResponse({'status': 'error', 'message': 'Error fetching data'}, status=500)


@csrf_exempt
def receive_data(request):
    if request.method == 'POST':
        try:
            if request.content_type != 'application/json':
                return JsonResponse({'status': 'error', 'message': 'Content-Type must be application/json'}, status=400)

            data = json.loads(request.body)
            temperature = data.get('temperature')
            humidity = data.get('humidity')
            airQuality = data.get('airQuality') 

            if temperature is not None and humidity is not None:
                SensorData.objects.create(
                    temperature=temperature,
                    humidity=humidity,
                    airQuality=airQuality 
                )
                return JsonResponse({'status': 'success'})
            else:
                return JsonResponse({'status': 'error', 'message': 'Missing temperature or humidity'}, status=400)

        except json.JSONDecodeError:
            return JsonResponse({'status': 'error', 'message': 'Invalid JSON'}, status=400)
        except Exception as e:
            print(f"[ERROR] receive_data: {e}")
            return JsonResponse({'status': 'error', 'message': str(e)}, status=500)

    return JsonResponse({'status': 'error', 'message': 'Only POST allowed'}, status=405)




@csrf_exempt  
def upload_firmware(request):
    static_dir = os.path.join(os.path.dirname(__file__), 'static')
    firmware_path = os.path.join(static_dir, 'firmware.ino.bin')
    version_file_path = os.path.join(static_dir, 'version.txt')

    if request.method == 'POST':
        form = FirmwareUploadForm(request.POST, request.FILES)

        if form.is_valid():
            firmware_file = request.FILES['firmware_file']

            
            os.makedirs(static_dir, exist_ok=True)

            
            with open(firmware_path, 'wb+') as destination:
                for chunk in firmware_file.chunks():
                    destination.write(chunk)

            
            current_version = "0"
            if os.path.exists(version_file_path):
                with open(version_file_path, "r") as f:
                    current_version = f.read().strip()

            new_version = "1" if current_version == "0" else "0"

            with open(version_file_path, "w") as f:
                f.write(new_version)

            return render(request, "success.html", {"version": new_version})
        else:
            return render(request, 'upload_firmware.html', {'form': form, 'error': 'Invalid form data'})

    else:
        form = FirmwareUploadForm()

    return render(request, 'upload_firmware.html', {'form': form})
