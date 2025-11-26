# Socks 5 Proxy

## Members

| Name                   | Student ID | Email                      |
|------------------------|------------|----------------------------|
| Lucia Oliveto          | 64646      | loliveto@itba.edu.ar       |
| Máximo Wehncke         | 64018      | mwehncke@itba.edu.ar       |
| Tomas Pietravallo      | 64288      | tpietravallo@itba.edu.ar   |
| Lorenzo Chiossone      | 64359      | lchiossone@itba.edu.ar     |

## Development

To compile the project, run: 

```sh
docker compose up --build compiler
```

To run tests, run:

```sh
docker compose up --build test
```

To benchmark against an nginx server:

```sh
docker compose up --build benchmark
```

To serve the generated JMeter HTML report on port 80 after a benchmark run, add the `show-results` service:

```sh
docker compose up --build benchmark show-results
```

## Report

The report for this assignment can be found in [REPORT.md](./REPORT.md) (in Spanish).

## Contributing

Some general contributing guidelines and CI/CD documentation can be found in [CONTRIBUTING.md](./CONTRIBUTING.md)
