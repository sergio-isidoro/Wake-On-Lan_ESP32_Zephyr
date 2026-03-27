#ifndef PORTAL_HTML_H
#define PORTAL_HTML_H

static const char html_p1[] =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>WOL Setup</title><style>"
    "body{margin:0;min-height:100vh;display:flex;align-items:center;"
    "justify-content:center;background:#0f0f0f;font-family:monospace;color:#e0e0e0}"
    ".box{background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:32px;width:280px}"
    "h2{margin:0 0 24px;font-size:18px;color:#00e5ff;letter-spacing:2px;text-align:center}";

static const char html_p2[] =
    "label{font-size:11px;color:#888;letter-spacing:1px;display:block;margin-bottom:4px}"
    "input[type=text],input[type=password]{width:100%;box-sizing:border-box;"
    "background:#111;border:1px solid #333;border-radius:6px;padding:10px;"
    "color:#fff;font-family:monospace;font-size:13px;margin-bottom:4px;outline:none}"
    "input:focus{border-color:#00e5ff}"
    ".pw-wrap{position:relative;margin-bottom:16px}"
    ".pw-wrap input{margin-bottom:0;padding-right:40px}"
    ".eye{position:absolute;right:10px;top:50%;transform:translateY(-50%);"
    "cursor:pointer;color:#888;font-size:15px;user-select:none}"
    ".eye:hover{color:#00e5ff}";

static const char html_p3[] =
    ".err{background:#2a0a0a;border:1px solid #ff3333;border-radius:6px;"
    "padding:10px;margin-bottom:16px;font-size:11px;color:#ff6666;text-align:center}"
    "input[type=submit]{width:100%;background:#00e5ff;color:#000;border:none;"
    "border-radius:6px;padding:12px;font-family:monospace;font-size:13px;"
    "font-weight:bold;letter-spacing:2px;cursor:pointer;margin-top:8px}"
    "input[type=submit]:hover{background:#fff}"
    "</style></head><body><div class='box'>"
    "<h2><svg width='18' height='18' viewBox='0 0 24 24' fill='#00e5ff' "
    "style='vertical-align:middle;margin-right:6px'>"
    "<path d='M1 9l2 2c5-5 13-5 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 "
    "8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 "
    "10 0l2-2C15.14 9.14 8.87 9.14 5 13z'/></svg>WOL CONFIG</h2>";

static const char html_p4[] =
    "<form action='/save' method='POST'>"
    "<label>SSID</label>"
    "<input type='text' name='s' autocomplete='off'>"
    "<label>PASSWORD</label>"
    "<div class='pw-wrap'>"
    "<input type='password' name='p' id='pw'>"
    "<span class='eye' onclick=\"var i=document.getElementById('pw');"
    "i.type=i.type=='password'?'text':'password';"
    "this.textContent=i.type=='password'?'&#128065;':'&#128064;';"
    "\">&#128065;</span></div>"
    "<label>PC IP</label>"
    "<input type='text' name='i' placeholder='192.168.1.100'>"
    "<label>TARGET MAC</label>"
    "<input type='text' name='m' placeholder='AA:BB:CC:DD:EE:FF' "
    "pattern='[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:"
    "[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}'>";

static const char html_err[] =
    "<div class='err'>&#9888; All fields are required!<br>"
    "IP: 192.168.x.x | MAC: AA:BB:CC:DD:EE:FF</div>";

static const char html_tail[] =
    "<input type='submit' value='SAVE'>"
    "</form></div></body></html>";

static const char html_ok[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
    "Connection: close\r\n\r\n"
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<style>"
    "body{margin:0;min-height:100vh;display:flex;align-items:center;"
    "justify-content:center;background:#0f0f0f;font-family:monospace;color:#e0e0e0}"
    ".box{background:#1a1a1a;border:1px solid #333;border-radius:12px;"
    "padding:32px;width:280px;text-align:center}"
    "h2{color:#00e5ff;letter-spacing:2px;font-size:16px}"
    "p{color:#888;font-size:12px}"
    "</style></head>"
    "<body><div class='box'>"
    "<h2>&#10003; SAVED!</h2>"
    "<p>Rebooting...</p>"
    "</div></body></html>";

#endif /* PORTAL_HTML_H */