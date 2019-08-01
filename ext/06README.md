ffplay -f avfoundation  -i "0" -framerate 30 -video_size 1280x720 -pixel_format uyvy422

```log
[avfoundation @ 0x7f9af586c800] Selected pixel format (yuv420p) is not supported by the input device.
[avfoundation @ 0x7f9af586c800] Supported pixel formats:
[avfoundation @ 0x7f9af586c800]   uyvy422
[avfoundation @ 0x7f9af586c800]   yuyv422
[avfoundation @ 0x7f9af586c800]   nv12
[avfoundation @ 0x7f9af586c800]   0rgb
[avfoundation @ 0x7f9af586c800]   bgr0
[avfoundation @ 0x7f9af586c800] Overriding selected pixel format to use uyvy422 instead.
Input #0, avfoundation, from '0':  0KB vq=    0KB sq=    0B f=0/0
  Duration: N/A, start: 15973.383900, bitrate: N/A
    Stream #0:0: Video: rawvideo (UYVY / 0x59565955), uyvy422, 1280x720, 29.97 tbr, 1000k tbn, 1000k tbc
```
