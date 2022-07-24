#script for downloading & processing the gitlab-ce docker container traces
import os
import time

version_set = ["14.0.0-ce.0","14.0.1-ce.0","14.0.10-ce.0","14.0.11-ce.0","14.0.12-ce.0","14.0.2-ce.0","14.0.3-ce.0","14.0.4-ce.0","14.0.5-ce.0","14.0.6-ce.0","14.0.7-ce.0","14.0.8-ce.0","14.0.9-ce.0","14.1.0-ce.0","14.1.1-ce.0","14.1.2-ce.0","14.1.3-ce.0","14.1.4-ce.0","14.1.5-ce.0","14.1.6-ce.0","14.1.7-ce.0","14.1.8-ce.0","14.10.0-ce.0","14.10.1-ce.0","14.10.2-ce.0","14.2.0-ce.0","14.2.1-ce.0","14.2.2-ce.0","14.2.3-ce.0","14.2.4-ce.0","14.2.5-ce.0","14.2.6-ce.0","14.2.7-ce.0","14.3.0-ce.0","14.3.1-ce.0","14.3.2-ce.0","14.3.3-ce.0","14.3.4-ce.0","14.3.5-ce.0","14.3.6-ce.0","14.4.0-ce.0","14.4.1-ce.0","14.4.2-ce.0","14.4.3-ce.0","14.4.4-ce.0","14.4.5-ce.0","14.5.0-ce.0","14.5.1-ce.0","14.5.2-ce.0","14.5.3-ce.0","14.5.4-ce.0","14.6.0-ce.0","14.6.1-ce.0","14.6.2-ce.0","14.6.3-ce.0","14.6.4-ce.0","14.6.5-ce.0","14.6.6-ce.0","14.6.7-ce.0","14.7.0-ce.0","14.7.1-ce.0","14.7.2-ce.0","14.7.3-ce.0","14.7.4-ce.0","14.7.5-ce.0","14.7.6-ce.0","14.7.7-ce.0","14.8.0-ce.0","14.8.1-ce.0","14.8.2-ce.0","14.8.3-ce.0","14.8.4-ce.0","14.8.5-ce.0","14.8.6-ce.0","14.9.0-ce.0","14.9.1-ce.0","14.9.2-ce.0","14.9.3-ce.0","14.9.4-ce.0"]

package_flag = "-cvf"
download_output_folder = "./download_container_gitlab/" # for output folder
repack_output_folder = "./packed-gitlab/" # for output folder

def Download(input_version):
    cmd = "bash download-frozen-image-v2.sh " + download_output_folder + " " + "gitlab/gitlab-ce:" + input_version 
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