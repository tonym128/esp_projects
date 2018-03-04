#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "FS.h"

const char* ssid = "FreESP Wifi";
 
int ledPin = 2; // GPIO1
IPAddress apIP(10, 10, 10, 1); // Private network for server
ESP8266WebServer server(80);
MDNSResponder mdns;
int hitCounter = 0;

// DNS server
const char *myHostname = "freesp";
const byte DNS_PORT = 53;
DNSServer dnsServer;
bool ledOutput = false;

void setup() {
  Serial.begin(115200);
  delay(10);
 
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledPin ? LOW : HIGH);
 
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid); // WiFi name

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/led", handleLight);  //Light on and off
  server.on("/ledswitch", handleLEDswitch);
  server.on("/hits", handleHits);
  server.onNotFound ( handleNotFound );

  server.begin();

  if (mdns.begin(myHostname, apIP)) {
    Serial.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
  }
  
  Serial.println("Server started");
  initHitCounter();
}
 
void loop() {
  // Match the request
  dnsServer.processNextRequest();
  
  //HTTP
  server.handleClient();
}

void initHitCounter() {
  SPIFFS.begin();
  
  if (!SPIFFS.exists("/freesp.txt")) {
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
   
    File f = SPIFFS.open("/freesp.txt", "w");
    if (!f) {
        Serial.println("file open failed");
    } else {
        f.println("Format Complete");
        f.close();
    }
  } else {
    Serial.println("SPIFFS is formatted.");
    hitCounter = loadHitCounter();
  }
}

int loadHitCounter() {
    File f = SPIFFS.open("/hitCounter.txt", "r");
    String s = f.readStringUntil('\n');
    char charBuf[50];
    s.toCharArray(charBuf, 50);
    f.close();
    return atoi(charBuf);
}

void saveHitCounter(int hits) {
    File f = SPIFFS.open("/hitCounter.txt", "w");
    char buffer[20];
    itoa(hits, buffer, 10);

    f.println(buffer);
    f.close();
}

void handleHits() {
    char buffer[20];
    itoa(hitCounter, buffer, 10);
    String page = "<!DOCTYPE HTML>";
    page += "<html>";
    page += "<head>";
    page += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
    page += "<title>Hits</title>";
    page += "<style>";
    page += "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\"";
    page += "</style>";
    page += "</head>";
    page += "<body>";
    page += "<h1>Hits</h1>";
    page += buffer;
    page += "</body>";
    page += "</html>";
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", page);
}

void handleLight() {
    digitalWrite(ledPin, ledOutput ? LOW : HIGH);
    
    char light_html[] =
    "<!DOCTYPE HTML>" 
    "<html>" 
    "<head>" 
    "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">" 
    "<title>Light Control</title>" 
    "<style>" 
    "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\"" 
    "</style>" 
    "</head>" 
    "<body>" 
    "<h1>Light Control</h1>" 
    "<a href=\"../ledswitch\">LED Switch</a><p>" 
    "</body>" 
    "</html>";
    
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/html", light_html);
}

void handleLEDswitch() {
  ledOutput = !ledOutput;
  server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()) + String("/led"), true);
  server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
}

void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200,"text/html","");
  server.sendContent("<canvas id=\"c\"></canvas><script>b=c.getContext`2d`;S=String.fromCharCode</script><script>c.style.width=c.style.height=`100%`,c.style.position=`fixed`,document.body.style.background=`radial-gradient(#023,#000)`,document.body.style.font=`0 a`;for(x=0;x<64;x++)c.width=c.height=12,b.shadowBlur=4,b.fillStyle=b.shadowColor=x%11?`#cff`:`#f26`,b.globalAlpha/=x<40?1:4,b.fillText(`  ESP ][ HAX0R  `[x]||S(9584+x),2,8),c[x]=b.createPattern(c,`repeat`);c.height=384,g=new AudioContext,p=g.createScriptProcessor(2048,c.style.top=c.style.left=a=m=0,1),p.connect(g.destination),p.onaudioprocess=o=>{for(u=o.outputBuffer.getChannelData(0),d=Math.sin(m/32),e=Math.cos(4*Math.cos(4*d)),f=Math.sin(4*Math.cos(4*d)),c.width=12*64,c.style.transform=`perspective(64vh)rotate3d(0,1,${f},${m/8}deg)scale(${1+(m/96)**64})`;a/64+64<8*m;b[i+64]=1&(-4&d*64)>>(4*b[i-1]+b[i]*2+b[i++])^1.01*Math.random())i=a++%2048;for(i=y=0;y<32;y++)for(x=0;x<64;x++)m+=1/g.sampleRate,j=2^y/5|1^x/20?(n=x/8*e-4*e-y/8*f+4*f+u[i]/4,o=x/8*f-4*f+y/8*e-4*e+16*d-8,n=0<o&[n%o,n&o+e*m,m+b[384|n+(o<<6)],n%o][3&m/24],b[i]|n&&(y^x&m)%16+40-20*n):12==y&&x<4*m?x-20:0,j&&b.fillRect(12*x,12*y,12,12,b.fillStyle=c[j^1.01*Math.random()]),u[i++]=(j/8+`9584`[3&m/24]*8)*m%1*d*d;b.globalAlpha/=x<40?1:4,b.drawImage(c,12*24,12*12,12,12,0,12*12,12*64,12),b.drawImage(c,12*12,12*24,12*3,24,0,0,12*64,384),3&m/24&&(h=speechSynthesis.speak(h,h.rate=0))};h=new SpeechSynthesisUtterance(`no free wifi for you`);</script>");
  server.client().stop(); // Stop is needed because we sent no content length 
  
  hitCounter += 1;
  saveHitCounter(hitCounter);
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  
  return false;
}

boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
