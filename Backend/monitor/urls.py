from django.urls import path
from . import views


# urls.py
urlpatterns = [

    path('data/', views.receive_data, name='receive_data'),
    path('', views.dashboard, name='dashboard'),
    path('get_all_data/', views.get_all_data, name='get_all_data'),
    path('upload-firmware/', views.upload_firmware, name='upload_firmware'),
    path('success/', views.success_page, name='success')

    
]
