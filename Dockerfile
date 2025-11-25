FROM ubuntu:latest AS runner

# Installs dependencies.
RUN apt-get update --yes \
	&& apt-get install --yes \
		gcc \
		make \
        clang-format \
		check \
		pkg-config \
	&& rm --force --recursive /var/lib/apt/lists/*
	