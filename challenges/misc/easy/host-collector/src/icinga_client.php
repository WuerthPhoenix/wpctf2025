<?php

/**
 * Standalone Icinga 2 REST API Client
 * No external dependencies
 */
class IcingaClient
{
    private $host;
    private $port;
    private $username;
    private $password;
    private $scheme;
    private $verifySsl;
    
    public function __construct($host, $username, $password, $port = 5665, $scheme = 'https', $verifySsl = false)
    {
        $this->host = $host;
        $this->port = $port;
        $this->username = $username;
        $this->password = $password;
        $this->scheme = $scheme;
        $this->verifySsl = $verifySsl;
    }
    
    public function get($endpoint)
    {
        $url = sprintf('%s://%s:%d/v1/%s', $this->scheme, $this->host, $this->port, ltrim($endpoint, '/'));

        $ch = curl_init();
        curl_setopt_array($ch, [
            CURLOPT_URL => $url,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_CUSTOMREQUEST => 'GET',
            CURLOPT_HTTPHEADER => [
                'Accept: application/json',
            ],
            CURLOPT_USERPWD => $this->username . ':' . $this->password,
            CURLOPT_TIMEOUT => 30,
            CURLOPT_SSL_VERIFYPEER => $this->verifySsl,
            CURLOPT_SSL_VERIFYHOST => $this->verifySsl ? 2 : 0
        ]);

        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $error = curl_error($ch);
        curl_close($ch);

        if ($error) {
            throw new Exception("cURL error: $error");
        }

        $decoded = json_decode($response, true);
        if (json_last_error() !== JSON_ERROR_NONE) {
            throw new Exception("Invalid JSON response: " . json_last_error_msg());
        }

        return new IcingaResponse($httpCode, $decoded);
    }

    /**
     * Make HTTP request to Icinga API
     */
    public function post($endpoint, $data = null)
    {
        $url = sprintf('%s://%s:%d/v1/%s', $this->scheme, $this->host, $this->port, ltrim($endpoint, '/'));
	
        $ch = curl_init();
        curl_setopt_array($ch, [
            CURLOPT_URL => $url,
            CURLOPT_RETURNTRANSFER => true,
            CURLOPT_CUSTOMREQUEST => 'POST',
            CURLOPT_HTTPHEADER => [
                'Accept: application/json',
                'Content-Type: application/json'
            ],
            CURLOPT_USERPWD => $this->username . ':' . $this->password,
            CURLOPT_TIMEOUT => 30,
            CURLOPT_SSL_VERIFYPEER => $this->verifySsl,
            CURLOPT_SSL_VERIFYHOST => $this->verifySsl ? 2 : 0
        ]);
        
        if ($data) {
            curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($data));
        }
        
        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        $error = curl_error($ch);
        curl_close($ch);
        
        if ($error) {
            throw new Exception("cURL error: $error");
        }
        
        $decoded = json_decode($response, true);
        if (json_last_error() !== JSON_ERROR_NONE) {
            throw new Exception("Invalid JSON response: " . json_last_error_msg());
        }
        
        return new IcingaResponse($httpCode, $decoded);
    }
}

/**
 * Icinga API Response wrapper
 */
class IcingaResponse
{
    private $httpCode;
    private $data;
    
    public function __construct($httpCode, $data)
    {
        $this->httpCode = $httpCode;
        $this->data = $data;
    }
    
    public function succeeded()
    {
        return $this->httpCode >= 200 && $this->httpCode < 300;
    }
    
    public function getResults()
    {
        return isset($this->data['results']) ? $this->data['results'] : null;
    }

    public function getSingleResult()
    {
        if (isset($this->data['results']) && is_array($this->data['results']) && count($this->data['results']) > 0) {
            return $this->data['results'][0];
        }
        return null;
    }
    
    public function getSingleStatusCode()
    {
        $result = $this->getSingleResult();
        return isset($result['code']) ? $result['code'] : $this->httpCode;
    }
    
    public function getRaw($key = null)
    {
        if ($key === null) {
            return $this->data;
        }
        return isset($this->data[$key]) ? $this->data[$key] : null;
    }
    
    public function getHttpCode()
    {
        return $this->httpCode;
    }
}
