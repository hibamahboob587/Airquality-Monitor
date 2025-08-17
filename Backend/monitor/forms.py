from django import forms

class FirmwareUploadForm(forms.Form):
    firmware_file = forms.FileField(label="Upload Firmware (.bin)", allow_empty_file=False)
