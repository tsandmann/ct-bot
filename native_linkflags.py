from platformio.util import get_systype

Import("env")

if "windows" in get_systype():
    env.Append(
        LIBS=[
            "ws2_32"
        ]
    )
