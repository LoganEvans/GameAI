# GameAI
This code is designed to allow a local AI to play against the connect4 AI found
at https://waywardtrends.com/bryan/FourInARow/Setup.html. The code does this by
first opening up a web page connected to the remote server and manipulating the
web page to input moves and wait for server moves. The code exposes this as an
API through a local server. The local AI logic then connects to this local
server to make its moves.

# Dependencies
    pip install selenium
    pip install splinter[selenium]

Install `bazel` by following the instructions at https://bazel.build/install.

# Start a game
    $ cd GameAi
    $ bazel build -c opt //brokers:connect4 //ais:connect4Client
    $ bazel-bin/brokers/connect4 &
    $ bazel-bin/ais/connect4Client
