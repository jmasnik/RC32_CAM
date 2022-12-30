
var websocket;
var servo_last_send = 0;
var servo_slider = 0;

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
   setInterval(servoInterval, 250);
   initWebSocket();
}

function setMotor(value){
   if(websocket.readyState == 1){
      console.log("Sending motor: " + value);
      websocket.send('M' + value);
   }
}

function setLight(value){
   if(websocket.readyState == 1){
      console.log("Sending light: " + value);
      websocket.send('L' + value);
   }
}

function setServo(event){
   servo_slider = event.srcElement.value;
}

function servoInterval(){
   if(servo_slider != servo_last_send && websocket.readyState == 1){
      console.log("Sending servo: " + servo_slider);
      websocket.send('S' + servo_slider);
      servo_last_send = servo_slider;
   }
}