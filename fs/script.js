
var gateway = `ws://192.168.0.192/ws`;
var websocket;

window.addEventListener('load', onLoad);

function initWebSocket() {
   console.log('Trying to open a WebSocket connection...');
   websocket = new WebSocket(gateway);
   websocket.onopen    = onOpen;
   websocket.onclose   = onClose;
   websocket.onmessage = onMessage;
}

function onOpen(event) {
   console.log('Connection opened');
}

function onClose(event) {
   console.log('Connection closed');
   setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
   console.log(typeof event.data);
   if(typeof event.data === 'object'){
      console.log(event.data.size);

      var urlCreator = window.URL || window.webkitURL;
      var imageUrl = urlCreator.createObjectURL(event.data);
      document.getElementById("image").src = imageUrl;      
   }
   if(typeof event.data === 'string'){
      console.log("STR: " + event.data);
   }
}

function onLoad(event) {
   initWebSocket();
}

function setMotor(value){
   websocket.send('M' + value);
}

function setLight(value){
   websocket.send('L' + value);
}

function setServo(event){
   websocket.send('S' + event.srcElement.value);
}