import sys
import argparse
import time
import random
import string
import logging
logging.getLogger('msrest.serialization').addHandler(logging.NullHandler())

from azure.batch import BatchServiceClient
from azure.batch.batch_auth import SharedKeyCredentials
import azure.batch.models as batchmodels
import azure.storage.blob as azureblob
import azure.batch.batch_service_client as batch
import azure.batch.batch_auth as batchauth


def randomString(n):
     return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(n))


def escape(s):
    return  ''.join(["\\{}".format(c) for c in s])


if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--account', required=True)
    parser.add_argument('--key', required=True)
    parser.add_argument('--url', required=True)
    parser.add_argument('--cmd', required=True)
    parser.add_argument('--jobid', required=True)
    args = parser.parse_args()

    _BATCH_ACCOUNT_NAME = args.account
    _BATCH_ACCOUNT_KEY  = args.key
    _BATCH_ACCOUNT_URL  = args.url
    cmd_string          = args.cmd
    job_name            = args.jobid
    task_name           = randomString(8)

    credentials = batchauth.SharedKeyCredentials(_BATCH_ACCOUNT_NAME,
                                                 _BATCH_ACCOUNT_KEY)

    batch_client = batch.BatchServiceClient(
        credentials,
        base_url=_BATCH_ACCOUNT_URL)
    
    task_cmd = "/bin/bash -c {} ".format(escape(cmd_string))

    # Add a task
    task = batch.models.TaskAddParameter(task_name, task_cmd)
    batch_client.task.add(job_name, task)

    # Wait for task to complete
    while True:
        task = batch_client.task.get(job_name, task_name)
        if task.state == batchmodels.TaskState.completed: break
        time.sleep(2)
    
    # Read stdout
    stdout_chunks = batch_client.file.get_from_task(
            job_name, task_name,
            'stdout.txt')
    stderr_chunks = batch_client.file.get_from_task(
            job_name, task_name,
            'stderr.txt')
    sys.stdout.write(''.join(stdout_chunks))
    sys.stderr.write(''.join(stderr_chunks))

