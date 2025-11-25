<?php
// PingChecker v2.1 - Enhanced Security Edition
// Validate request method
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    header('Location: index.php');
    exit;
}

$hostname = $_POST['hostname'] ?? '';

if (empty($hostname)) {
    die(renderError('No hostname provided'));
}

// Enhanced security filters - Force users to use & for command injection
$blocked_patterns = [
    ';',        // semicolon
    '&&',       // double ampersand  
    '||',       // double pipe
    '|',        // single pipe
    '$(',       // command substitution
    '${',       // variable substitution
    '`',        // backticks
    '\n',       // newline character
    '\r',       // carriage return
    '%0a',      // URL-encoded newline (lowercase)
    '%0A',      // URL-encoded newline (uppercase)
    '%0d',      // URL-encoded carriage return (lowercase)
    '%0D',      // URL-encoded carriage return (uppercase)
    '\x0a',     // hex newline
    '\x0d',     // hex carriage return
    'cat',      // direct file reading
    'more',     // file reading
    'less',     // file reading  
    'head',     // file reading
    'tail',     // file reading
    'strings',  // binary file reading
    'od',       // octal dump
    'xxd',      // hex dump
    'hexdump',  // hex dump
    'base64',   // encoding/decoding
    'flag',     // obvious flag reference
    'FLAG',     // uppercase flag reference
    'nc',       // netcat
    'netcat',   // netcat full
    'bash',     // shell spawning
    'sh',       // shell spawning
    'python',   // scripting languages
    'perl',     // scripting languages
    'ruby',     // scripting languages
    'php',      // PHP execution
    'wget',     // file download
    'curl',     // file download
];

// Check for blocked patterns
foreach ($blocked_patterns as $pattern) {
    if (stripos($hostname, $pattern) !== false) {
        die(renderError("Security violation detected! Pattern not allowed: " . htmlspecialchars($pattern)));
    }
}

// Additional check for newline characters in different encodings
if (preg_match('/[\r\n\x0a\x0d]/i', $hostname)) {
    die(renderError("Invalid characters detected in hostname"));
}

// Check for URL encoding attempts (manual decoding check)
$decoded = urldecode($hostname);
if ($decoded !== $hostname) {
    // Check if decoded version contains newlines
    if (preg_match('/[\r\n]/i', $decoded)) {
        die(renderError("URL-encoded newlines are not allowed"));
    }
}

// Limit hostname length to prevent buffer issues
if (strlen($hostname) > 100) {
    die(renderError("Hostname too long (max 100 characters)"));
}

// Function to render error messages in CTF style
function renderError($message) {
    return renderOutput("ERROR", $message, true);
}

// Function to render output in CTF style
function renderOutput($title, $content, $isError = false) {
    $statusColor = $isError ? '#FF0000' : '#53AF33';
    $statusIcon = $isError ? '✗' : '✓';
    
    $html = <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
    <title>PingChecker - Results</title>
    <link href="/style.css" rel="stylesheet" />
    <meta http-equiv="content-type" content="text/html; charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
</head>
<body>
    <div class="jumbotron">
        <div class="container">
            <h1 class="max-800px text-center"></h1>
        </div>
    </div>
     <div class="container">
        <div class="row">
            <div class="col-md-8 col offset-md-2 col-lg-6 offset-lg-3">
                <div class="p-2 mb-2" style="background: rgba(255,191,0,.1)">
                <div class="result-header">
                    <span>[ {$title} ]</span>
                    <span class="status-icon">{$statusIcon}</span>
                </div>
                <pre class="result-content">{$content}</pre>
HTML;

    // Add developer hint only on successful ping results
    if (!$isError && strpos($content, 'PING') !== false) {
        $html .= <<<HTML
            
            <!-- Developer Notes - v2.1 -->
            <!-- TODO: Move old configuration backups from /var/backups/ -->
            <!-- REMINDER: Database location referenced in system.conf -->
            <!-- DEBUG: Logging system active at /opt/logs/ -->
            
                <div class="hint-box">
                    <strong>System Status:</strong> Operation completed successfully.<br>
                    Check system configuration for additional diagnostic options.
                </div>
HTML;
    }
    
    $html .= <<<HTML
                </div>  
                <a href="index.php" class="btn btn-primary">« RETURN TO SCANNER</a>
            </div>
        </div>
    </div>
</body>
</html>
HTML;
    
    return $html;
}

// Build and execute the ping command
$command = "ping -c 4 " . $hostname;

// Execute command with timeout and capture both stdout and stderr
$descriptorspec = array(
    0 => array("pipe", "r"),  // stdin
    1 => array("pipe", "w"),  // stdout
    2 => array("pipe", "w")   // stderr
);

$process = proc_open($command, $descriptorspec, $pipes);

if (is_resource($process)) {
    // Close stdin
    fclose($pipes[0]);
    
    // Set timeout
    stream_set_timeout($pipes[1], 5);
    stream_set_timeout($pipes[2], 5);
    
    // Read output
    $stdout = stream_get_contents($pipes[1]);
    $stderr = stream_get_contents($pipes[2]);
    
    fclose($pipes[1]);
    fclose($pipes[2]);
    
    $return_value = proc_close($process);
    
    // Combine output
    $output = $stdout;
    if (!empty($stderr)) {
        $output .= "\n--- STDERR ---\n" . $stderr;
    }
    
    // Display the output
    if ($output) {
        echo renderOutput("PING RESULTS", htmlspecialchars($output));
    } else {
        echo renderError("No output received or command failed");
    }
} else {
    echo renderError("Failed to execute command");
}
?>
