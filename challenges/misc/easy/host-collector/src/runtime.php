<?php

require_once __DIR__ . '/icinga_client.php';

class IcingaRuntime
{
    public $client;
    
    public function __construct($host = null, $user = null, $pass = null)
    {
        $host = $host ?: getenv('ICINGA_HOST') ?: 'localhost';
        $user = $user ?: getenv('ICINGA_USER') ?: 'root';
        $pass = $pass ?: getenv('ICINGA_PASS') ?: 'icinga';
        $port = getenv('ICINGA_PORT') ?: 5665;
        
        $this->client = new IcingaClient($host, $user, $pass, $port, 'https', false);
    }
    
    public function createObjectAtRuntime($objectName, $address = "127.0.0.1", $echoCommand = false)
    {
        
        $definition = sprintf('object Host "%s" { check_command = "hostalive"; address = "%s" }', $objectName, $address);
        
        $command = sprintf(
            "f = function() {\n%s\n}\nInternal.run_with_activation_context(f)",
            $definition
        );
        
        // For testing: return the command instead of executing it
        if ($echoCommand) {
            echo $command;
        }
        
        return $this->client->post('console/execute-script', ['command' => $command]);
    }
}

if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/') {
    header('Content-Type: text/html');
    echo file_get_contents('/opt/index.html');
    exit;
}


if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/fonts/W95FA.woff') {
    header('Content-Type: font/woff');
    readfile('/opt/fonts/W95FA.woff');
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/fonts/pxiKyp0ihIEF2isfFJU.woff2') {
    header('Content-Type: font/woff2');
    readfile('/opt/fonts/pxiKyp0ihIEF2isfFJU.woff2');
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/fonts/sysfont.woff') {
    header('Content-Type: font/woff');
    readfile('/opt/fonts/sysfont.woff');
    exit;
}


if ($_SERVER['REQUEST_METHOD'] === 'GET' && $_SERVER['REQUEST_URI'] === '/icinga/get_hosts') {
    header('Content-Type: application/json');

    try {
        $api = new IcingaRuntime(
            "localhost",
            getenv('ICINGA_USER'),
            getenv('ICINGA_PASS')
        );

        $result = $api->client->get('objects/hosts');

        if ($result->succeeded()) {
            $hosts = $result->getResults();
            $response = [];
            foreach ($hosts as $host) {
                $response[] = [
                    'name' => $host['name'],
                    'address' => $host['attrs']['address'],
                    'check_command' => $host['attrs']['check_command'],
                ];
            }
            echo json_encode(['success' => true, 'hosts' => $response]);
        } else {
            http_response_code(500);
            echo json_encode([
                'error' => 'Failed to get hosts',
                'http_code' => $result->getHttpCode(),
                'icinga_response' => $result->getRaw(),
            ]);
        }
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => $e->getMessage()]);
    }
    exit;
}


if ($_SERVER['REQUEST_METHOD'] === 'POST' && $_SERVER['REQUEST_URI'] === '/icinga/create_host') {
    header('Content-Type: application/json');


    try {
        $input = json_decode(file_get_contents('php://input'), true);

  	if (!$input || !isset($input['object_name'])) {
            http_response_code(400);
            echo json_encode(['error' => 'object_name is required']);
            exit;
        }
        
	$objectName = $input['object_name'];
	$objectAddress = $input['address'];
        
        $api = new IcingaRuntime(
            "localhost",
            getenv('ICINGA_USER'),
            getenv('ICINGA_PASS'),
	);
        
	$result = $api->createObjectAtRuntime($objectName, $objectAddress, false);
        
        if ($result->succeeded()) {
            $response = $result->getSingleResult();
            if ($response === "Object already exists") {
                http_response_code(409);
                echo json_encode(['error' => 'Object already exists', 'object_name' => $objectName]);
            } else {
                echo json_encode(['success' => true, 'object_name' => $objectName, 'host' => 'localhost', 'response' => $response]);
            }
        } else {
            $errorData = [
                'error' => 'Failed to create object',
                'object_name' => $objectName,
                'http_code' => $result->getHttpCode(),
                'icinga_response' => $result->getRaw(),
                'status_code' => $result->getSingleStatusCode()
            ];
            
            // Add specific error message if available
            $singleResult = $result->getSingleResult();
            if ($singleResult && isset($singleResult['status'])) {
                $errorData['icinga_error'] = $singleResult['status'];
            }
            
            http_response_code(500);
            echo json_encode($errorData);
        }
        
    } catch (Exception $e) {
        http_response_code(500);
        echo json_encode(['error' => $e->getMessage()]);
    }
    exit;
}

http_response_code(404);
echo 'Not Found';