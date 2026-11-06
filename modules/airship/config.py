def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "AccountManager",
        "AgonesManager",
        "AgonesPendingRequest",
        "AirshipAudioRequest",
        "AirshipAudioStreaming",
        "AirshipBufferRequest",
        "AirshipBufferStreaming",
        "AirshipCloudBuffer",
        "AirshipLoadTiming",
        "AirshipMeshRequest",
        "AirshipMeshStreaming",
        "AirshipTextureRequest",
        "AirshipTextureStreaming",
        "BaseSpring",
        "HTTPRequest",
        "PCKCache",
        "PCKRequest",
        "PIDController",
        "PackageManager",
        "PlatformURL",
        "ProcessSignalManager",
        "ResourceRequest",
        "SceneManager",
        "SimpleHTTPRequest",
        "SimpleHTTPResponse",
        "Spring1D",
        "Spring2D",
        "Spring3D",
        "User",
    ]


def get_doc_path():
    return "doc_classes"
