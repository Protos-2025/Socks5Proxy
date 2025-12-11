# Socks 5 Proxy

## Members

| Name                   | Student ID | Email                      |
|------------------------|------------|----------------------------|
| Lucia Oliveto          | 64646      | loliveto@itba.edu.ar       |
| Máximo Wehncke         | 64018      | mwehncke@itba.edu.ar       |
| Tomas Pietravallo      | 64288      | tpietravallo@itba.edu.ar   |
| Lorenzo Chiossone      | 64359      | lchiossone@itba.edu.ar     |

## Development

### Make

You may compile this project by running:

```sh
make
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
