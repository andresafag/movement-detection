import json
import logging
import urllib.parse
import boto3
import uuid
 

# Initialize logger and AWS clients
logger = logging.getLogger()
logger.setLevel(logging.INFO)

iot_client = boto3.client('iot')
s3_client = boto3.client('s3')

# Define your AWS Role ARN that allows IoT to read from S3
ROLE_ARN = "arn:aws:iam::688567305851:role/sensor-movement"

def handler(event, context):
    # Imprime el evento recibido para facilitar la depuración en CloudWatch Logs
    logger.info(f"Full received event: {json.dumps(event)}")
    
    try:
        # 1. Parse S3 bucket and object key depending on the event source
        if 'detail' in event:
            # Estructura cuando el evento viene de EventBridge
            bucket_name = event['detail']['bucket']['name']
            raw_key = event['detail']['object']['key']
            object_key = urllib.parse.unquote_plus(raw_key)
        elif 'Records' in event:
            # Estructura tradicional cuando se prueba con la plantilla nativa de S3
            s3_record = event['Records'][0]['s3']
            bucket_name = s3_record['bucket']['name']
            object_key = urllib.parse.unquote_plus(s3_record['object']['key'])
        else:
            raise KeyError("Formato de evento desconocido. No se encontró 'detail' ni 'Records'.")
        
        logger.info(f"Triggered by file: s3://{bucket_name}/{object_key}")
        
        # 2. Extract or define your IoT Target (Thing Name or Thing Group ARN)
        target_thing_name = "esp32-sensor-01" 
        target_arn = f"arn:aws:iot:us-east-1:688567305851:thing/{target_thing_name}"
        
        # 3. Create a unique Job ID
        clean_key = object_key.replace('/', '-').replace('.', '-')
        short_id = uuid.uuid4().hex[:6]
        job_id = f"ota-job-{clean_key}-{short_id}"[:64]
        
        logger.info(f"Creating IoT OTA Job: {job_id} for target: {target_arn}")
        
        presigned_url = s3_client.generate_presigned_url(
            'get_object',
            Params={'Bucket': bucket_name, 'Key': object_key},
            ExpiresIn=3600
        )

        job_document = {
            "execution": {
                "jobId": job_id,
                "executionNumber": 1,
                "jobDocument": {
                    "url": presigned_url
                }
            }
        }
        # 4. Create the OTA Update Job
        response = iot_client.create_job(
            jobId=job_id,
            targets=[target_arn],
            document=json.dumps(job_document).replace("\\/", "/"),
            description="Automated firmware OTA update via S3 presigned URL.",
            targetSelection='SNAPSHOT'
        )
        
        logger.info(f"OTA Update Job created successfully. Response: {json.dumps(response, default=str)}")
        
        return {
            'statusCode': 200,
            'body': json.dumps(f"Successfully created OTA Job {job_id}")
        }

    except Exception as e:
        logger.error(f"Error processing OTA trigger: {str(e)}")
        raise e
