var gateway
if (window.location.port) {
    gateway = `ws://${window.location.hostname}:${window.location.port}/ws`;
} else {
    gateway = `ws://${window.location.hostname}/ws`;
}  
var showing_reset = false;
var modal = document.getElementById("modal");
var modal_text = document.getElementById("modal-text");
var wserror = true;
var linerror = false
var doingreset = false;

function ShowModal() {
    if (wserror) {
        modal_text.textContent="Connecting, please wait";
        modal.style.display="block";
        return;
    }
    if (linerror) {
        modal_text.textContent="Lin bus error";
        modal.style.display="block";
        return;
    }
    if (doingreset) {
        modal_text.textContent="Resetting, please wait";
        modal.style.display="block";
        return;
    }
    modal.style.display="none";
}

ShowModal();

function EnableFanSpeed(enable) {
    document.getElementById("fanspeed").disabled=!enable;
}

function EnableFan() {
    heating=document.getElementById("heating").checked;
    EnableFanSpeed(!heating);
    enabled=document.getElementById("boiler").value=="off" || heating;
    document.getElementById("fan").disabled=!enabled;  
}

function ShowResetButton() {
    button=document.getElementById("error_reset_button");
    if (showing_reset && !doingreset ) {
        button.style.visibility="visible";
   } else {
        button.style.visibility="hidden";
   }
}
    // WebSocket connection
    const ws = new ReconnectingWebSocket(gateway);
    ws.debug=true;

    function Ping() {
        ws.send("ping");
        setTimeout(Ping,10000);
    }

    setTimeout(Ping,10000);

    // Function to update control values
    function updateControl(id, value) {
        const control = document.getElementById(id);
        if (control) {
            if (control.type=="checkbox") {
                control.checked=parseInt(value);
            } else {
                control.value = value;
            }
            if (id=="boiler" || id=="heating") {
                EnableFan();
            }
        }
    }

    // Function to handle incoming WebSocket messages
    ws.onmessage = function (event) {
        //console.log("==ws message: "+event.data);
        const data = JSON.parse(event.data);
        if (data.command && data.id && data.value) {
            id=data.id.replace('/','')
            if (data.command=='setting')
              updateControl(id, data.value);
            else
              updateDetails(id, data.value);
        }
    };

    ws.onopen = function (event) {
        wserror=false;
        ShowModal();
        ws.send("settings");
    };

    ws.onclose = function (event) {
        wserror=true;
        ShowModal();
    };

    // Function to update details in the Details tab
     function updateDetails(name, value) {
        v=document.getElementById(name);
        if (v) {
            if (v.type=="checkbox") {
                v.checked=parseInt(value);
            } else {
                v.textContent=value;
            }
        }
        if (name == "linok") {
            linerror=parseInt(value)!=1;
            ShowModal();
        }
        if (name == "reset") {
            doingreset=parseInt(value)==1;
            ShowModal();
            ShowResetButton();
        }
        if (name=="err_class") {
          err_class=parseInt(value);
          elem=document.getElementById("error_class");
          switch(err_class) {
                case 0:
                  elem.textContent="";  
                  showing_reset=false;
                  break;
                case 1:
                case 2:
                  elem.textContent="Warning";
                  showing_reset=false;
                  break;
                case 10:
                case 20:
                case 30:
                  elem.textContent="Error";
                  showing_reset=true;
                  break;
                case 40:
                  elem.textContent="Locked";      
                  showing_reset=false;
                  break;
                default:
                  elem.textContent="???";
                  showing_reset=true;
          }
          ShowResetButton();
        }
        if (name=="err_code") {
            code=parseInt(value);
            if (code==0) {
                document.getElementById("error_code").textContent="";
                document.getElementById("error_msg").textContent="";
            } else {
                document.getElementById("error_code").textContent=value;
                msg=ErrText[code];
                if (msg) {
                    document.getElementById("error_msg").textContent=msg;
                } else {
                    document.getElementById("error_msg").textContent="unknown error code";
                }
            }
        }
        if (name=="waterboost") {
            if (parseInt(value)==0) {
                document.getElementById("waterboost_msg").textContent="";
            } else {
                document.getElementById("waterboost_msg").textContent="waterboost "+value+" minutes remaining";
            }
        }
        const detailsTable = document.getElementById('detailsTable');
        let updated = false;
        for (let i = 1; i < detailsTable.rows.length; i++) {
            if (detailsTable.rows[i].cells[0].textContent === name) {
                detailsTable.rows[i].cells[1].textContent = value;
                updated = true;
                break;
            }
        }
        if (!updated) {
            const newRow = detailsTable.insertRow(-1);
            newRow.insertCell(0).textContent = name;
            newRow.insertCell(1).textContent = value;
        }
    }

    // Function to send control selections back via WebSocket
    function sendControlSelection(id, value) {
        if (ws.readyState == 1) {
            const message = JSON.stringify({ id: id, value: value });
            ws.send(message);
        }
    }

    // Function to switch tabs
    window.openTab = function (evt, tabName) {
        var i, tabcontent, tablinks;

        // Hide all tab content
        tabcontent = document.getElementsByClassName("tabcontent");
        for (i = 0; i < tabcontent.length; i++) {
            tabcontent[i].style.display = "none";
        }

        // Deactivate all tab links
        tablinks = document.getElementsByClassName("tablinks");
        for (i = 0; i < tablinks.length; i++) {
            tablinks[i].className = tablinks[i].className.replace(" active", "");
        }

        // Show the specific tab content and mark the button as active
        document.getElementById(tabName).style.display = "block";
        evt.currentTarget.className += " active";
    };

    // Open the default tab (Controls tab)
    document.querySelector('.tablinks.active').click();

    // Event listeners for control selections
    document.getElementById('boiler').addEventListener('change', function () {
        sendControlSelection('/boiler', this.value);
        EnableFan();
    });

    var tempTimer;

    function ValidateAndSetTemperature() {
        elem=document.getElementById('temp');
        val = parseFloat(elem.value);
        if (val<5.0) {
            val=5.0;
        };
        if (val>30.0) {
            val=30.0;
        };
        elem.value = val.toFixed(1).toString();
        sendControlSelection('/temp', elem.value);
    }

    function changeTemp(increment) {
      clearTimeout(tempTimer);
      t=document.getElementById('temp');
      val=parseFloat(t.value)+increment;
      if (val<5.0) {
        val=5.0;
      }
      if (val>30.0) {
        val=30.0;
      }
      t.value=val.toFixed(1).toString();
      tempTimer = setTimeout(ValidateAndSetTemperature, 800);
    }

    document.getElementById('temp').addEventListener('input', function() {
            clearTimeout(tempTimer);
            tempTimer = setTimeout(ValidateAndSetTemperature, 800);
    });

    document.getElementById('heating').addEventListener('change', function () {
        if (this.checked) 
          sendControlSelection('/heating','1');
        else
          sendControlSelection('/heating', '0');
        EnableFan();
    });

    document.getElementById('fan').addEventListener('change', function () {
        sendControlSelection('/fan', this.value);
    });

    document.getElementById('error_reset_button').addEventListener('click', function () {
        sendControlSelection('/error_reset',1);
    });
