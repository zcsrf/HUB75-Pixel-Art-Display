const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">

<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta charset="UTF-8">
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.6/dist/css/bootstrap.min.css" rel="stylesheet"
        integrity="sha384-4Q6Gf2aSP4eDXB8Miphtr37CMZZQ5oXLH2yaXMJ2w8e2ZtHTl7GptT4jmndRuHDT" crossorigin="anonymous">
    <title>HUB75 Pixel-Art-Display</title>
</head>
<body>
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.6/dist/js/bootstrap.bundle.min.js"
        integrity="sha384-j1CDi7MgGQ12Z7Qab0qlWQ/Qqz24Gc6BM0thvEMVjHnfYGF0rmFCozFSxQBxwHKO"
        crossorigin="anonymous"></script>
    <script src=https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js></script>
    <div class="container">
        <header class="d-flex flex-wrap justify-content-center py-3 mb-4 border-bottom">
          <a href="/" class="d-flex align-items-center mb-3 mb-md-0 me-md-auto text-dark text-decoration-none">
            <svg class="bi me-2" width="40" height="32"></svg>
            <span class="fs-4">HUB75 Pixel-Art-Display</span>
          </a>
        </header>
      </div>
    <div class='container' style=margin-top:50px>
        <div class="card-group">
            <div class="card">
                <div class="card-header">
                    <h5>Device Firmware</h5>
                </div>
                <div class="card-body">
                    <div id=vr class="text-center">
                        Version: 25
                    </div>
                </div>
            </div>
            <div class="card">
                <div class="card-header">
                    <h5>Used Flash</h5>
                </div>
                <div class="card-body">
                    <div id=uf class="text-center">
                        Free: 10Mb
                    </div>
                </div>
            </div>
            <div class="card">
                <div class="card-header">
                    <h5>Free Flash</h5>
                </div>
                <div class="card-body">
                    <div id=ff class="text-center">
                        Free: 10Mb
                    </div>
                </div>
            </div>
            <div class="card">
                <div class="card-header">
                    <h5>Total Flash</h5>
                </div>
                <div class="card-body">
                    <div id=tf class="text-center">
                        Free: 10Mb
                    </div>
                </div>
            </div>
        </div>
        <br>
        <div class="card">
            <div class="card-header">
                <h5>Actions</h5>
            </div>
            <div class="card-body">
                <div class="row">
                    <div class="col-sm-3">
                        <div class="form-check form-switch">
                            <input class="form-check-input" type="checkbox" id="play-gif-check" checked
                                onchange="toggleGIF(this)">
                            <label class="form-check-label" for="flexSwitchCheckChecked">Play Gif</label>
                        </div>
                    </div>
                    <div class="col-sm-3">
                        <div class="form-check form-switch">
                            <input class="form-check-input" type="checkbox" id="loop-gif-check" checked
                                onchange="toggleLoopGif(this)">
                            <label class="form-check-label" for="flexSwitchCheckChecked">Loop Gif</label>
                        </div>
                    </div>
                    <div class="col-sm-3">
                        <div class="form-check form-switch">
                            <input class="form-check-input" type="checkbox" id="show-clock-check" checked
                                onchange="toggleClock(this)">
                            <label class="form-check-label" for="flexSwitchCheckChecked">Show Clock</label>
                        </div>
                    </div>
                    <div class="col-sm-3">
                        <div class="form-check form-switch">
                            <input class="form-check-input" type="checkbox" id="show-text-check" checked
                                onchange="toggleScrollText(this)">
                            <label class="form-check-label" for="flexSwitchCheckChecked">Show Text</label>
                        </div>
                    </div>
                </div>

                <br>
                <div id='ds'>Brightness: 50</div>
                <input id="dss" type='range' class="form-range" min='0' max='255' value='50'
                    onchange='updateSliderPWM(this.value)' />
            </div>
        </div>
        <br>
        <div class="card">
            <div class="card-header">
                <h5>Scroll Text</h5>
            </div>
            <div class="card-body">
                <div class="row">
                    <div class="col-sm-6">
                        <div class="input-group mb-3">
                            <span class="input-group-text">Color Picker</span>
                            <input type="color" class="form-control form-control-color" id="stc" value="#563d7c"
                                title="Choose your color" onchange="setColor(this.value)">
                        </div>
                        <div class="input-group mb-3">
                            <label class="input-group-text" for="inputGroupSelect01">Text Size</label>
                            <select class="form-select" id="stf" onchange='sendFontSize(this.value)'>
                                <option selected>Choose...</option>
                                <option value="1">Small</option>
                                <option value="2" selected>Normal</option>
                                <option value="3">Big</option>
                                <option value="4">Huge</option>
                            </select>
                        </div>
                        <div id='sr'>Scroll Speed: 50
                        </div>
                        <input id="srs" type='range' class="form-range" min='0' max='150' value='50'
                            onchange='sendScrollSpeed(this.value)' />
                    </div>
                    <div class="col-sm-6">
                        <div class="input-group h-100 w-100 p-3">
                            <span class="input-group-text">Scroll Text</span>
                            <textarea class="form-control" id="stt" onchange='sendScrollText(this.value)'></textarea>
                            <button class="btn btn-outline-secondary" type="button" id="button-addon2">Send</button>
                        </div>
                        <br>

                    </div>
                </div>
            </div>
        </div>
        <br>
        <div class="card">
            <div class="card-header">
                <h5>Upload File</h5>
            </div>
            <div class="card-body">
                <div class="container">
                    <div class="row" id="upload">
                        <div class="input-group h-100 w-100 p-3">
                            <input class="form-control" type="file" id="formFile" accept=".gif"
                                onchange="fileLoad(event)">
                            <button class="btn btn-outline-secondary" type="button" id="button-addon2"
                                onclick="uploadFile()">Upload</button>
                        </div>
                        <div class="row justify-content-center" id="preview"></div>
                    </div>
                </div>
            </div>
        </div>
        <br>
        <div class="card">
            <div class="card-header">
                <h5>Gallery</h5>
            </div>
            <div class="card-body">
                <div class="container">
                    <div class="row" id="gallery"></div>
                </div>
            </div>
            <br>
        </div>
        <br>

        <footer class="py-3 my-4">
            <p class="text-center text-body-secondary">HUB75 Pixel Art display by <a
                href="https://github.com/zcsrf/HUB75-Pixel-Art-Display" target="_blank">zcsrf</a> forked from <a
                    href="https://github.com/mzashh/HUB75-Pixel-Art-Display/" target="_blank">mzashh</a></p>
        </footer>

        <script>
            function rgbToHex(r, g, b) {
                return "#" + (1 << 24 | r << 16 | g << 8 | b).toString(16).slice(1);
            }
            function hexToRgb(hex) {
                var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
                return result ? {
                    r: parseInt(result[1], 16),
                    g: parseInt(result[2], 16),
                    b: parseInt(result[3], 16)
                } : null;
            }

            function formatBytes(bytes, decimals = 2) {
                if (!+bytes) return '0 Bytes'

                const k = 1024
                const dm = decimals < 0 ? 0 : decimals
                const sizes = ['Bytes', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB', 'EiB', 'ZiB', 'YiB']

                const i = Math.floor(Math.log(bytes) / Math.log(k))

                return `${parseFloat((bytes / Math.pow(k, i)).toFixed(dm))} ${sizes[i]}`
            }

            function vf() {
                $.getJSON('/j/vf', function (t) {
                    var vr = 'Version ' + t.v;
                    var ff = 'Free ' + t.ff;
                    var uf = 'Used ' + t.uf;
                    var tf = 'Total ' + t.tf;

                    $('#vr').html(vr);
                    $('#uf').html(uf);
                    $('#tf').html(tf);
                    $('#ff').html(ff);

                    if (t.gs == '1') { document.getElementById("play-gif-check").checked = true; }
                    else { document.getElementById("play-gif-check").checked = false; }

                    if (t.ls == '1') { document.getElementById("loop-gif-check").checked = true; }
                    else { document.getElementById("loop-gif-check").checked = false; }

                    if (t.cs == '1') { document.getElementById("show-clock-check").checked = true; }
                    else { document.getElementById("show-clock-check").checked = false; }

                    if (t.ts == '1') { document.getElementById("show-text-check").checked = true; }
                    else { document.getElementById("show-text-check").checked = false; }

                    var dsText = 'Brightness :' + t.ds;
                    $('#ds').html(dsText);
                    document.getElementById('dss').value = t.ds;

                    var srText = 'Scroll Speed: ' + t.sts;
                    $('#sr').html(srText);
                    document.getElementById('srs').value = t.sts;

                    document.getElementById("stf").value = t.stf;

                    document.getElementById("stc").value = rgbToHex(t.str, t.stg, t.stb);

                    document.getElementById('stt').value = t.stt;


                    console.log(t)
                })
            }

            function loadGallery() {
                fetch('/listFilesAlt')
                    .then(response => response.json())
                    .then(images => {
                        const gallery = document.getElementById('gallery');
                        gallery.innerHTML = ''; // Clear existing
                        images.forEach(url => {
                            const col = document.createElement('div');
                            col.className = 'col-6 col-md-4 col-lg-3 mb-4';
                            col.innerHTML = `
                <div class="card shadow-sm">
                  <img src="/file?name=${url}&action=show\" class="card-img-top img-thumb" alt="Image" loading="lazy">
                  <div class="card-body text-center">
                  <button class="btn btn-sm btn-primary me-1" onclick="playImage('${url}')">Play</button>
                  <a class="btn btn-sm btn-success me-1" href="/file?name=${url}&action=show" download">Download</a>
                  <button class="btn btn-sm btn-danger" onclick="deleteImage('${url}')">Delete</button>
                </div>
                </div>`;
                            gallery.appendChild(col);
                        });
                    })
                    .catch(err => console.error('Error loading images:', err));
            }

            vf();
            loadGallery();
        </script>
        <script>
            function playImage(filename) {
                const url = `/file?name=${filename}&action=play`;
                fetch(url)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert(`Playing GIF: ${filename}`);
                    })
                    .catch(error => {
                        console.error("Error playing GIF:", error);
                        alert("Failed to play GIF.");
                    });
            }

            function deleteImage(url) {
                const filename = `/file?name=${url}&action=delete`;
                fetch(filename)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("File deleted successfully!");
                        loadGallery(); // Reload the page to update the file list
                    })
                    .catch(error => {
                        console.error("Error deleting file:", error);
                        alert("Failed to delete file.");
                    });
            }
        </script>
        <script>
            function updateSliderPWM(value) {
                var dsText = 'Brightness :' + value;
                $('#ds').html(dsText);
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/slider?value=" + value, true);
                xhr.send();
            }

            function setColor(value) {
                console.log(value);
                const r = hexToRgb(value).r;
                const g = hexToRgb(value).g;
                const b = hexToRgb(value).b;

                fetch(`/setColor?r=${r}&g=${g}&b=${b}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Color updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error:", error);
                        alert("Failed to update color.");
                    });
            }
            function toggleGIF(checkbox) {
                const state = checkbox.checked ? "on" : "off";
                fetch(`/toggleGIF?state=${state}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("GIF playback state updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error:", error);
                        alert("Failed to update GIF playback state.");
                    });
            }
            function toggleLoopGif(checkbox) {
                const state = checkbox.checked ? "on" : "off";
                fetch(`/toggleLoopGif?state=${state}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert(`Loop GIF ${state === "on" ? "enabled" : "disabled"}`);
                    })
                    .catch(error => {
                        console.error("Error toggling Loop GIF:", error);
                        alert("Failed to toggle Loop GIF.");
                    });
            }
            function toggleClock(checkbox) {
                const state = checkbox.checked ? "on" : "off";

                // Automatically disable scrolling text if the clock is enabled
                if (state === "on") {
                    document.getElementById("show-text-check").checked = false;
                    toggleScrollText(document.getElementById("show-text-check"));
                }

                fetch(`/toggleClock?state=${state}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Clock state updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error:", error);
                    });
            }

            function toggleScrollText(checkbox) {
                const isEnabled = checkbox.checked;

                /*const scrollTextInput = document.getElementById("scrollText");
                const fontSizeToggle = document.getElementById("fontSizeToggle");
                const scrollSpeedInput = document.getElementById("scrollSpeed");

                // Enable or disable the text input, font size toggle, and speed input
                scrollTextInput.disabled = !isEnabled;
                fontSizeToggle.disabled = !isEnabled;
                scrollSpeedInput.disabled = !isEnabled;*/

                // Automatically disable the clock if scrolling text is enabled
                if (isEnabled) {
                    document.getElementById("show-clock-check").checked = false;
                    toggleClock(document.getElementById("show-clock-check"));
                }

                // Send the scrolling text state to the firmware
                fetch(`/toggleScrollText?state=${isEnabled ? "on" : "off"}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert(`Scrolling text ${isEnabled ? "enabled" : "disabled"}`);
                    })
                    .catch(error => {
                        console.error("Error toggling scrolling text:", error);
                        alert("Failed to toggle scrolling text.");
                    });
            }

            function sendFontSize(value) {
                // Send the text, font size, and speed to the firmware
                fetch(`/updateScrollText?fontSize=${value}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Scrolling text updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error updating scrolling text:", error);
                        alert("Failed to update scrolling text.");
                    });
            }

            function sendScrollSpeed(value) {
                // Send the text, font size, and speed to the firmware
                fetch(`/updateScrollText?speed=${value}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Scrolling text updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error updating scrolling text:", error);
                        alert("Failed to update scrolling text.");
                    });
            }

            function sendScrollText(value) {
                // Send the text, font size, and speed to the firmware
                fetch(`/updateScrollText?text=${encodeURIComponent(value)}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Scrolling text updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error updating scrolling text:", error);
                        alert("Failed to update scrolling text.");
                    });
            }

            function sendScrollTextData() {
                const text = document.getElementById("scrollText").value;
                const fontSize = document.getElementById("fontSizeToggle").value;
                const speed = document.getElementById("scrollSpeed").value;

                // Send the text, font size, and speed to the firmware
                fetch(`/updateScrollText?text=${encodeURIComponent(text)}&fontSize=${fontSize}&speed=${speed}`)
                    .then(response => response.text())
                    .then(data => {
                        console.log(data);
                        alert("Scrolling text updated successfully!");
                    })
                    .catch(error => {
                        console.error("Error updating scrolling text:", error);
                        alert("Failed to update scrolling text.");
                    });
            }

            function rebootButton() {
                document.getElementById("fileList").innerHTML = "Invoking Reboot ...";
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/reboot", false);
                xhr.send();
                window.open("/reboot", "_self");
            }

            function downloadDeleteButton(filename, action) {
                console.log(`downloadDeleteButton called with filename: ${filename}, action: ${action}`);

                if (action === "delete") {
                    const url = `/file?name=${filename}&action=delete`;
                    fetch(url)
                        .then(response => response.text())
                        .then(data => {
                            console.log(data);
                            alert("File deleted successfully!");
                            location.reload(); // Reload the page to update the file list
                        })
                        .catch(error => {
                            console.error("Error deleting file:", error);
                            alert("Failed to delete file.");
                        });
                } else if (action === "download") {
                    const url = `/file?name=${filename}&action=download`;
                    window.open(url, "_blank"); // Open the file in a new tab for download
                } else if (action === "play") {
                    const url = `/file?name=${filename}&action=play`;
                    fetch(url)
                        .then(response => response.text())
                        .then(data => {
                            console.log(data);
                            alert(`Playing GIF: ${filename}`);
                        })
                        .catch(error => {
                            console.error("Error playing GIF:", error);
                            alert("Failed to play GIF.");
                        });
                }
            }

            function fileLoad(event) {
                if (event.target.files[0]) {
                    preview.innerHTML = ''; // Clear existing
                    const col = document.createElement('div');
                    col.className = 'col-6 col-md-4 col-lg-3 mb-4';
                    col.innerHTML = `
                <div class="card shadow-sm">
                  <img id="previewOut" src="" class="card-img-top img-thumb" alt="Image">
                  <div class="card-body text-center" id="previewText">
                </div>
                </div>`;
                    preview.appendChild(col);

                    var previewOut = document.getElementById('previewOut');
                    previewOut.src = URL.createObjectURL(event.target.files[0]);
                    previewOut.onload = function (ie) {
                        var text = '' + this.naturalHeight + 'x' + this.naturalWidth + " - " + formatBytes(event.target.files[0].size);
                        previewText.innerHTML = text;
                        URL.revokeObjectURL(previewOut.src) // free memory
                    }
                }
                else {
                    preview.innerHTML = ''; // Clear existing
                }
            };

            function showUploadButtonFancy() {
                document.getElementById("detailsheader").innerHTML = "<h3>Upload File<h3>"
                document.getElementById("status").innerHTML = "";
                var uploadform = "<form method = \"POST\" action = \"/upload\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"/><input type=\"submit\" name=\"upload\" value=\"Upload\" title = \"Upload File\"></form>"
                document.getElementById("fileList").innerHTML = uploadform;
                var uploadform =
                    "<form id=\"upload_form\" enctype=\"multipart/form-data\" method=\"post\">" +
                    "<input type=\"file\" name=\"file1\" id=\"file1\" onchange=\"uploadFile()\"><br>" +
                    "<progress id=\"progressBar\" value=\"0\" max=\"100\" style=\"width:300px;\"></progress>" +
                    "<h3 id=\"status\"></h3>" +
                    "<p id=\"loaded_n_total\"></p>" +
                    "</form>";
                document.getElementById("fileList").innerHTML = uploadform;
            }

            function uploadFile() {
                console.log(document.getElementById("formFile").files[0]);


                var file = document.getElementById("formFile").files[0];
                // alert(file.name+" | "+file.size+" | "+file.type);
                var formdata = new FormData();
                formdata.append("file1", file);

                var ajax = new XMLHttpRequest();
                ajax.upload.addEventListener("progress", progressHandler, false);
                ajax.addEventListener("load", completeHandler, false); // doesnt appear to ever get called even upon success
                ajax.addEventListener("error", errorHandler, false);
                ajax.addEventListener("abort", abortHandler, false);
                ajax.open("POST", "/upload");
                ajax.send(formdata);

            }
            function progressHandler(event) {
                console.log('Loaded: ' + event.loaded + ' - Total ' + event.total)
                //_("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total; // event.total doesnt show accurate total file size
                /*
                _("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes";
                var percent = (event.loaded / event.total) * 100;
                _("progressBar").value = Math.round(percent);
                _("status").innerHTML = Math.round(percent) + "% uploaded... please wait";
                if (percent >= 100) {
                    _("status").innerHTML = "Please wait, writing file to filesystem";
                }*/
            }
            function completeHandler(event) {
                console.log('Upload Complete');
                
                var previewOut = document.getElementById('previewOut');
                preview.innerHTML = ''; // Clear existing
                var picker = document.getElementById("formFile");
                picker.value = '';

                loadGallery();
                

                /*
                _("status").innerHTML = "Upload Complete";
                _("progressBar").value = 0;
                xmlhttp = new XMLHttpRequest();
                xmlhttp.open("GET", "/listfiles", false);
                xmlhttp.send();
                document.getElementById("status").innerHTML = "File Uploaded";
                document.getElementById("detailsheader").innerHTML = "<h3>Files<h3>";
                document.getElementById("fileList").innerHTML = xmlhttp.responseText;
                */
            }
            function errorHandler(event) {
                console.log('Upload Failed');

                // _("status").innerHTML = "Upload Failed";
            }
            function abortHandler(event) {
                console.log('inUpload Aborted');

                //_("status").innerHTML = "inUpload Aborted";
            }
        </script>
</body>

</html>
)rawliteral";
