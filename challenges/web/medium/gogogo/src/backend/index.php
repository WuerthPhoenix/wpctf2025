<?php

session_start();

if(!isset($_SESSION["sandbox_id"]))
    $_SESSION["sandbox_id"] = bin2hex(random_bytes(32));

$sandbox = '/sandbox/' . md5("wpctf" . $_SESSION["sandbox_id"]);
@mkdir($sandbox);
@chdir($sandbox) or die("err?");

if($_SERVER["REQUEST_METHOD"] == "POST" && isset($_GET["filename"])){
    file_put_contents( $sandbox."/".md5($_GET["filename"]), file_get_contents("php://input"));
    exit();
      
}

if($_SERVER["REQUEST_METHOD"] == "GET" && isset($_GET["filename"])){
    $filename = $sandbox."/".md5($_GET["filename"]);
    if(file_exists($filename)){
        $finfo = finfo_open(FILEINFO_MIME_TYPE);
        $mime  = finfo_file($finfo, $filename);
        finfo_close($finfo);
        header("Content-Type: $mime");

        echo file_get_contents($filename);
        exit();
    }
}

?>


<!DOCTYPE html>
<html lang="en">
<head>
    <link href="/style.css" rel="stylesheet" />
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File uploader</title>
</head>
<body>
    <div class="jumbotron">
        <div class="container">
            <h1 class="max-800px text-center">
                [ g0g0g0 uploader v0.1 ]
            </h1>
            <p>Easy uploader for private files</p>
        </div>
    </div>

    <div class="container">
        <div class="row">
            <div class="col-md-8 col offset-md-2 col-lg-6 offset-lg-3">

                <form action="index.php" method="POST">
                    <div class="form-group">
                        <label for="file">
                            File
                        </label>
                        <input type="file"
                               id="file"
                               name="file"
                               required
                               autocomplete="off"
                               class="form-control"
                        >
                        
                    </div>

                    <div class="form-group mt-2">
                        <label for="filename">
                            Filename
                        </label>
                        <input type="text"
                               id="filename"
                               name="filename"
                               required
                               autocomplete="off"
                               class="form-control"
                        >
                        
                    </div>

                    <button type="submit" class="btn btn-primary mt-3">
                        Upload
                    </button>
                </form>

                <a class="mt-5 info-box" style="display: none" id="info">
                    
                </a>

                <table class="mt-3 table table-striped">
                    <thead>
                        <tr>
                            <td>Uploaded files</td>
                        </tr>
                    </thead>
                    <tbody id="filelist"></tbody>
                </table>
            </div>
        </div>
    </div>
    <script>



function show_files(){
    filelist = JSON.parse(localStorage.getItem("filelist") || "[]");
    const table = document.getElementById("filelist");
    table.innerHTML = "";
    for(const f of filelist){
        const tr = document.createElement("tr");
        const td = document.createElement("td");
        const a = document.createElement("a");
        a.href = f.url;
        a.innerText = f.filename;
        td.appendChild(a);
        tr.appendChild(td);
        table.appendChild(tr);
    }
}

document.addEventListener("DOMContentLoaded", function(){
    show_files();
});

document.querySelector("form").addEventListener("submit", async function (e) {
    e.preventDefault(); 

    const fileInput = document.getElementById("file");
    const filenameInput = document.getElementById("filename");

    if (!fileInput.files.length) {
        alert("Seleziona un file!");
        return;
    }

    const file = fileInput.files[0];
    const filename = filenameInput.value.trim();
    if (!filename) {
        alert("Inserisci un filename!");
        return;
    }

    const arrayBuffer = await file.arrayBuffer();
    const url = "index.php?filename=" + encodeURIComponent(filename);
    const resp = await fetch(url, {
        method: "POST",
        body: arrayBuffer
    });

    if (resp.ok) {
       
        document.getElementById("info").href = url;
        document.getElementById("info").innerText = `Private link: ${url}`;
        document.getElementById("info").style.display = "block";
        

        /* save to local storage */


        let filelist = JSON.parse(localStorage.getItem("filelist") || "[]");
        filelist.push({filename: filename, url: url});
        localStorage.setItem("filelist", JSON.stringify(filelist));
        show_files();
    } else {
        alert("Error: " + resp.status);
    }
});
</script>
</body>
</html>