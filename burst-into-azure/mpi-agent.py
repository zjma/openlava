#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import argparse
import time
import random
import string
import os
import sys
import datetime
import logging
logging.getLogger('msrest.serialization').addHandler(logging.NullHandler())


from azure.batch import BatchServiceClient
from azure.batch.batch_auth import SharedKeyCredentials
import azure.batch.models as batchmodels
import azure.storage.blob as azureblob
import azure.batch.batch_service_client as batch
import azure.batch.batch_auth as batchauth


def print_batch_exception(batch_exception):
    """
    Prints the contents of the specified Batch exception.

    :param batch_exception:
    """
    print('-------------------------------------------')
    print('Exception encountered:')
    if batch_exception.error and \
            batch_exception.error.message and \
            batch_exception.error.message.value:
        print(batch_exception.error.message.value)
        if batch_exception.error.values:
            print()
            for mesg in batch_exception.error.values:
                print('{}:\t{}'.format(mesg.key, mesg.value))
    print('-------------------------------------------')


def get_vm_config_for_distro(batch_service_client, distro, version):
    """
    Gets a virtual machine configuration for the specified distro and version
    from the list of Azure Virtual Machines Marketplace images verified to be
    compatible with the Batch service.

    :param batch_service_client: A Batch service client.
    :type batch_service_client: `azure.batch.BatchServiceClient`
    :param str distro: The Linux distribution that should be installed on the
    compute nodes, e.g. 'Ubuntu' or 'CentOS'. Supports partial string matching.
    :param str version: The version of the operating system for the compute
    nodes, e.g. '15' or '14.04'. Supports partial string matching.
    :rtype: `azure.batch.models.VirtualMachineConfiguration`
    :return: A virtual machine configuration specifying the Virtual Machines
    Marketplace image and node agent SKU to install on the compute nodes in
    a pool.
    """
    # Get the list of node agents from the Batch service
    node_agent_skus = batch_service_client.account.list_node_agent_skus()

    # Get the first node agent that is compatible with the specified distro.
    # Note that 'distro' in this case actually maps to the 'offer' property of
    # the ImageReference.
    node_agent = next(agent for agent in node_agent_skus
                      for image_ref in agent.verified_image_references
                      if distro.lower() in image_ref.offer.lower() and
                      version.lower() in image_ref.sku.lower())

    # Get the last image reference from the list of verified references
    # for the node agent we obtained. Typically, the verified image
    # references are returned in ascending release order so this should give us
    # the newest image.
    img_ref = [image_ref for image_ref in node_agent.verified_image_references
               if distro.lower() in image_ref.offer.lower() and
               version.lower() in image_ref.sku.lower()][-1]

    # Create the VirtualMachineConfiguration, specifying the VM image
    # reference and the Batch node agent to be installed on the node.
    # Note that these commands are valid for a pool of Ubuntu-based compute
    # nodes, and that you may need to adjust the commands for execution
    # on other distros.
    vm_config = batchmodels.VirtualMachineConfiguration(
        image_reference=img_ref,
        node_agent_sku_id=node_agent.id)

    return vm_config



def randomString(n):
     return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(n))


def upload_file_to_container(block_blob_client, container_name, file_path):
    """
    Uploads a local file to an Azure Blob storage container.

    :param block_blob_client: A blob service client.
    :type block_blob_client: `azure.storage.blob.BlockBlobService`
    :param str container_name: The name of the Azure Blob storage container.
    :param str file_path: The local path to the file.
    :rtype: `azure.batch.models.ResourceFile`
    :return: A ResourceFile initialized with a SAS URL appropriate for Batch
    tasks.
    """
    blob_name = os.path.basename(file_path)

    block_blob_client.create_blob_from_path(container_name,
                                            blob_name,
                                            file_path)

    sas_token = block_blob_client.generate_blob_shared_access_signature(
        container_name,
        blob_name,
        permission=azureblob.BlobPermissions.READ,
        expiry=datetime.datetime.utcnow() + datetime.timedelta(hours=2))

    sas_url = block_blob_client.make_blob_url(container_name,
                                              blob_name,
                                              sas_token=sas_token)

    return batchmodels.ResourceFile(file_path=blob_name,
                                    blob_source=sas_url)


if __name__=='__main__':
    '''
    Run an OpenMPI program on Azure Batch, get back its stdout and stderr.

    A MPI program is an executable
    which may depend on libmpi.so or OpenMPI environment variables.
    '''
    parser = argparse.ArgumentParser()
    parser.add_argument('--batchAccount', required=True)
    parser.add_argument('--batchKey', required=True)
    parser.add_argument('--batchUrl', required=True)
    parser.add_argument('--storageAccount', required=True)
    parser.add_argument('--storageKey', required=True)
    parser.add_argument('--coreCount', required=True, type=int)
    parser.add_argument('--executable', required=True)
    parser.add_argument('--jobName', required=True)
    parser.add_argument('--taskName', required=True)

    args = parser.parse_args()
    
    credentials = batchauth.SharedKeyCredentials(
            args.batchAccount,
            args.batchKey)

    batch_client = batch.BatchServiceClient(
            credentials,
            base_url=args.batchUrl)

    job_name = args.jobName
    task_name = args.taskName
    coreCount = args.coreCount
    executable_path = args.executable
    executable_filename = os.path.basename(executable_path)

    # Send executable to azure storage.
    blob_client = azureblob.BlockBlobService(
            account_name=args.storageAccount,
            account_key=args.storageKey)

    blob_container_name = "mpi-tasks-blobs"

    blob_client.create_container(blob_container_name, fail_on_exist=False)

    common_res_file = upload_file_to_container(
            blob_client, blob_container_name, executable_path)

    # Submit a multi-instance task.
    task_cmd = "/usr/lib64/openmpi/bin/mpirun -np {} --host $AZ_BATCH_HOST_LIST -wdir $AZ_BATCH_TASK_SHARED_DIR $AZ_BATCH_TASK_SHARED_DIR/{}".format(coreCount, executable_filename)

    coordination_cmdline = "chmod +x $AZ_BATCH_TASK_SHARED_DIR/{}".format(executable_filename)

    mpiSettings = batch.models.MultiInstanceSettings(
            number_of_instances         = coreCount,
            coordination_command_line   = coordination_cmdline,
            common_resource_files       = [common_res_file])

    task = batch.models.TaskAddParameter(
            id                      = task_name,
            command_line            = task_cmd,
            display_name            = task_name,
            resource_files          = None,
            environment_settings    = None,
            affinity_info           = None,
            constraints             = None,
            run_elevated            = None,
            multi_instance_settings = mpiSettings,
            depends_on              = None)

    batch_client.task.add(job_name, task)

    #Wait for task to complete.
    while True:
        task = batch_client.task.get(job_name, task_name)
        if task.state == batchmodels.TaskState.completed: break
        time.sleep(2.0)

    #Read stderr.
    stderr_chunks = batch_client.file.get_from_task(
            job_name, task_name,
            'stderr.txt')
    sys.stderr.write(''.join(stderr_chunks))
    sys.stderr.flush()

    #Read stdout.
    stdout_chunks = batch_client.file.get_from_task(
            job_name, task_name,
            'stdout.txt')
    sys.stdout.write(''.join(stdout_chunks))
    sys.stdout.flush()

