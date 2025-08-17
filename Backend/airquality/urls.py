# airquality/urls.py
from django.contrib import admin
from django.urls import path, include
from monitor import views  # Import views from your app
from django.conf import settings
from django.conf.urls.static import static

urlpatterns = [
    #path('', TemplateView.as_view(template_name='index.html')),
    path('admin/', admin.site.urls),
    path('api/', include('monitor.urls')),  # Include your app's routes
    path('', views.dashboard, name='dashboard'), # Root path for homepage
]+ static(settings.STATIC_URL, document_root=settings.STATIC_ROOT)
