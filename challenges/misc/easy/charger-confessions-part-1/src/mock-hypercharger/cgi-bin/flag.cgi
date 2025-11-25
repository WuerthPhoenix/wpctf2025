#!/bin/bash

echo "Content-Type: text/html"
echo ""
echo "<html>"
echo "<head><title>Flag</title></head>"
echo "<body>"
echo "<h1>Congratulations!</h1>"
echo "<p>You found the flag:</p>"
echo "<pre>$FLAG</pre>"
echo "</body>"
echo "</html>"