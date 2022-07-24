#script for downloading & processing the couchbase docker container traces
import os
import time

version_set = ["2.5.2", "3.0.2", "3.0.3", "3.1.0", "3.1.3", "3.1.5", "3.1.6", "4.0.0", "4.1.0", "4.1.1", "4.5.0", "4.5.1", "4.6.0", "4.6.1", "4.6.2", "4.6.3", "4.6.4", "4.6.5", "5.0.1", "5.1.0", "5.1.1", "5.5.0", "5.5.1", "5.5.2", "6.0.0", "6.0.1", "6.0.2", "6.0.3", "6.0.4", "6.0.5", "6.5.0", "6.5.1", "6.6.0", "6.6.1", "6.6.2", "community-2.2.0", "community-3.0.1", "community-3.1.3", "community-4.0.0", "community-4.1.0", "community-4.1.1", "community-4.5.0", "community-4.5.1", "community-5.0.1", "community-5.1.1", "community-6.0.0", "community-6.5.0'  'community-6.5.1", "community-6.6.0", "community", "enterprise-2.5.2", "enterprise-3.0.2", "enterprise-3.0.3", "enterprise-3.1.0", "enterprise-3.1.3", "enterprise-3.1.5", "enterprise-3.1.6", "enterprise-4.0.0", "enterprise-4.1.0", "enterprise-4.1.1", "enterprise-4.5.0", "enterprise-4.5.1", "enterprise-4.6.0", "enterprise-4.6.1", "enterprise-4.6.2", "enterprise-4.6.3", "enterprise-4.6.4", "enterprise-4.6.5", "enterprise-5.0.1", "enterprise-5.1.0", "enterprise-5.1.1", "enterprise-5.5.0", "enterprise-5.5.1", "enterprise-5.5.2", "enterprise-6.0.0", "enterprise-6.0.1", "enterprise-6.0.2", "enterprise-6.0.3", "enterprise-6.0.4", "enterprise-6.0.5", "enterprise-6.5.0", "enterprise-6.5.1", "enterprise"]

package_flag = "-cvf"
download_output_folder = "./download_container_couch/" # for output folder
repack_output_folder = "./packed-couch/" # for output folder

def Download(input_version):
    cmd = "bash download-frozen-image-v2.sh " + download_output_folder + " " + "couchbase:" + input_version 
    print(cmd)
    os.system(cmd)

def Process(input_version):
    # re-pack the folder
    output_file_name = repack_output_folder + input_version + ".tar"
    cmd = "tar " + package_flag + " " + output_file_name + " " + download_output_folder
    print(cmd)
    os.system(cmd)
    # remove the download file
    cmd = "rm -rf " + download_output_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":
    make_dir_name = repack_output_folder
    if not os.path.exists(make_dir_name):
        os.mkdir(make_dir_name, 0o777)
    counter = 1
    for version in version_set:
        Download(version)
        Process(version)
        counter = counter + 1
        print("Download version: "+version+" done")
        if (counter % 80 == 1):
            print("Start sleep 6 hours")
            time.sleep(21600)