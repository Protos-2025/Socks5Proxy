FROM ubuntu:latest AS runner

# Installs dependencies.
RUN apt-get update --yes \
	&& apt-get install --yes \
		gcc \
		make \
        clang-format \
		clang-tidy \
		check \
		pkg-config \
		curl \
		iputils-ping \
		netcat-openbsd \
	&& rm --force --recursive /var/lib/apt/lists/*
	