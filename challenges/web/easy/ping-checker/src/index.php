<!DOCTYPE html>
<html lang="en">
<head>
    <link href="/style.css" rel="stylesheet" />
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Ping Checker</title>
</head>
<body>
    <div class="jumbotron">
        <div class="container">
            <h1 class="max-800px text-center">
                [ PingChecker v2.1 ]
            </h1>
            <p>Advanced connectivity testing utility for system administrators</p>
        </div>
    </div>

    <div class="container">
        <div class="row">
            <div class="col-md-8 col offset-md-2 col-lg-6 offset-lg-3">

                <form action="ping.php" method="POST">
                    <div class="form-group">
                        <label for="hostname">
                            <span class="terminal-prompt">$</span> Target Host/IP:
                        </label>
                        <input type="text"
                               id="hostname"
                               name="hostname"
                               placeholder="127.0.0.1 | example.com | 8.8.8.8"
                               required
                               autocomplete="off"
                               class="form-control"
                        >
                        <p class="example mt-2">
                            Examples: localhost, google.com, 192.168.1.1, 10.0.0.1
                        </p>
                    </div>

                    <button type="submit" class="btn btn-primary">
                        EXECUTE PING SCAN
                    </button>
                </form>

                <div class="mt-5 info-box">
                    <h4>System Information:</h4>
                    <ul>
                        <li>ICMP Echo Request Protocol Active</li>
                        <li>Maximum 4 packets per request</li>
                        <li>Response timeout: 5 seconds</li>
                        <li>Supported protocols: IPv4/IPv6</li>
                    </ul>
                    <p style="margin-top: 15px; font-size: 16px; opacity: 0.7;">
                        Service running on Ubuntu 22.04 LTS<br>
                        Last maintenance: Check system logs for details
                    </p>
                </div>
            </div>
        </div>
    </div>
</body>
</html>