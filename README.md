# Socks 5 Proxy

## Members

| Name                   | Student ID | Email                      |
|------------------------|------------|----------------------------|
| Lucia Oliveto          | 64646      | loliveto@itba.edu.ar       |
| Máximo Wehncke         | 64018      | mwehncke@itba.edu.ar       |
| Tomas Pietravallo      | 64288      | tpietravallo@itba.edu.ar   |
| Lorenzo Chiossone      | 64359      | lchiossone@itba.edu.ar     |

## Variables

You may modify some constants easily by defining environment variables. These will be picked up from a `.env` file in the project root. You can use the provided [`.env.sample`](./.env.sample) as a starting point.

Example `.env` file:

```sh
DEBUG=1
BUFFER_SIZE=8192
MAX_LOG_QUEUE_SIZE=1000
LOGGER_MIN_LEVEL=LOGGER_TRACE
```

Some of the available variables include:

| Variable              | Description                                                                                                                                   | Default Value   |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|-----------------|
| DEBUG                 | Enables debug compiler flags (-g, -fsanitize=address, etc)                                                                                    | `true`          |
| TRACE                 | Enables `bpftrace` monitoring for blocking syscalls when running server.sh (the `server` service entrypoint in docker)                        | `false`         |
| BUFFER_SIZE           | Size of the buffer used for reading/writing data between sockets                                                                              | `1024`          |
| MAX_LOG_QUEUE_SIZE    | Maximum number of log messages that can be queued before dropping logs                                                                        | `100`           |
| MAX_LOG_SIZE          | Maximum size (in bytes) of a single log message. Longer messages will be truncated with an ellipsis.                                          | `1024`          |
| LOGGER_MIN_LEVEL      | Minimum log level to be logged. Possible values: `LOGGER_TRACE`, `LOGGER_DEBUG`, `LOGGER_INFO`, `LOGGER_WARN`, `LOGGER_ERROR`, `LOGGER_FATAL` | `LOGGER_INFO`   |

All server variables and their default values can be found in [defines.h](./src/server/include/defines.h).

## Development

### Make

You may compile this project by running:

```sh
make clean all
```

Then run the binaries found in the `bin/` folder.

```sh
# Run the Socks5 proxy server
./bin/server
```

#### Tests

Compile tests by running

```sh
# runs make clean test & generated binaries
./scripts/test.sh
```

### Docker

To compile the server using docker, run:

```sh
docker compose up --build server
```

To run benchmarks and serve the generated JMeter HTML report on port 80 after a benchmark run:

```sh
docker compose up --build benchmark show-results
```

#### Tests

Compile tests by running

```sh
docker compose up --build test
```

## Usage

```sh
# FQDN Request example using curl
curl --proxy "socks5h://admin:password@localhost:1080" http://www.google.com
```

```sh
# Example using the nginx-test-server container
curl --proxy "socks5h://admin:password@localhost:1080" 10.0.0.111/test_file_01.txt
```

## Report

The report for this assignment can be found in [REPORT.md](./REPORT.md) (in Spanish).

## Contributing

Some general contributing guidelines and CI/CD documentation can be found in [CONTRIBUTING.md](./CONTRIBUTING.md)
