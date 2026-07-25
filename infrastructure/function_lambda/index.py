import json
import logging
import urllib.parse
import boto3

# Initialize logger and AWS clients
logger = logging.getLogger()
logger.setLevel(logging.INFO)

iot_client = boto3.client('iot')

# Define your AWS Role ARN that allows IoT to read from S3
ROLE_ARN = "arn:aws:iam::688567305851:role/sensor-movement"

def handler(event, context):
    try:
        # 1. Parse S3 bucket and object key from the event trigger
        s3_record = event['Records'][0]['s3']
        bucket_name = s3_record['bucket']['name']
        object_key = urllib.parse.unquote_plus(s3_record['object']['key'])
        
        logger.info(f"Triggered by file: s3://{bucket_name}/{object_key}")
        
        # 2. Extract or define your IoT Target (Thing Name or Thing Group ARN)
        target_thing_name = "esp32-sensor-01" 
        target_arn = f"arn:aws:iot:us-east-1:123456789012:thing/{target_thing_name}"
        
        # 3. Create a unique Job ID
        clean_key = object_key.replace('/', '-').replace('.', '-')
        job_id = f"ota-job-{clean_key}"[:64] # Max length is 64 characters
        
        logger.info(f"Creating IoT OTA Job: {job_id} for target: {target_arn}")
        
        # 4. Create the OTA Update Job
        response = iot_client.create_ota_update(
            otaUpdateId=job_id,
            description="Automated firmware OTA update triggered by S3 upload.",
            targets=[target_arn],
            targetSelection='SNAPSHOT', # Use SNAPSHOT for one-time, CONTINUOUS for dynamic groups
            awsJobExecutionsRolloutConfig={
                'maximumPerMinute': 10
            },
            files=[
                {
                    'fileLocation': {
                        's3Location': {
                            'bucket': bucket_name,
                            'key': object_key
                        }
                    },
                    # Optional: Add code signing if your firmware requires it
                    # 'codeSigning': { ... } 
                },
            ],
            roleArn=ROLE_ARN
        )
        
        logger.info(f"OTA Update Job created successfully. Response: {json.dumps(response, default=str)}")
        
        return {
            'statusCode': 200,
            'body': json.dumps(f"Successfully created OTA Job {job_id}")
        }

    except Exception as e:
        logger.error(f"Error processing OTA trigger: {str(e)}")
        raise e
