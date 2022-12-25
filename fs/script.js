
var websocket;

window.addEventListener('load', onLoad);

function initWebSocket() {
   var gateway;  
   if(window.location.protocol == 'file:'){
      gateway = 'ws://192.168.0.220/ws';
   } else {
      gateway = 'ws://' + window.location.hostname + '/ws';
   }
   console.log('Trying to open a WebSocket ' + gateway);
   websocket = new WebSocket(gateway);
   websocket.onopen = onOpen;
   websocket.onclose = onClose;
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
   if(typeof event.data === 'object'){
      //console.log(event.data.size);
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