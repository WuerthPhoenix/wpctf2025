import boto3
import json

s3_client = boto3.client('s3')


def lambda_handler(event, context):
    print("Event received:", json.dumps(event))  # Debugging line
    # Extract path from the event object
    path = event.get("rawPath", "/")  # Lambda URL sends rawPath
    body = event.get("body", "")

    # JEFF: The customer asked us to make the Lambda flexible enough to execute arbitrary commands to query resources in AWS. 
    # We implemented this as a 'temporary bypass' since the deadline was very close.
    if path == "/eval":
        # NOTE: This endpoint was added for internal testing purposes.
        try:
            # Evaluate the "code" parameter from the JSON body
            request_data = json.loads(body)
            code = request_data.get("code", "")
            if not code:
                return {"statusCode": 400, "body": "No code provided."}
            
            result = eval(code) 
            return {"statusCode": 200, "body": json.dumps({"result": str(result)})}
        except Exception as e:
            return {"statusCode": 500, "body": f"Error: {str(e)}"}

    elif path == "/health":
        try:
            return {"statusCode": 200, "body": "healthy - ready to eval!"}
        except Exception as e:
            return {"statusCode": 500, "body": f"Error: {str(e)}"}
    
    else:
        return {"statusCode": 404, "body": "Endpoint not found"}

