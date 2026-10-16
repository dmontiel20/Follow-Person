import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    pkg_dir = get_package_share_directory('follow_person')
    param_file = os.path.join(pkg_dir, 'config', 'params.yaml')

    detection2tf = Node(package='follow_person',
                        executable='detection_2_tf_main',
                        output='screen',
                        )

    followperson = Node(package='follow_person',
                        executable='follow_person_main',
                        output='screen',
                        )

    ld = LaunchDescription()
    ld.add_action(detection2tf)
    ld.add_action(followperson)

    return ld