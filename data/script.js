const startbtn = document.getElementById("startBtn");


function applyStateButton(btn, state) {
    if (state === "on") {
        btn.style.backgroundColor = "green";
        btn.textContent = "on";
    } else {
        btn.style.backgroundColor = "red";
        btn.textContent = "off";
    }
}


fetch("/GetStatus")
    .then(response => response.text())
    .then(data => {
        applyStateButton(startbtn, data);
        console.info("state :" + data);
    })
    .catch(err => {
        console.error("Error while fetching \"/GetStatus\" :" + err)
        applyStateButton(startbtn, "off");
    });
    
