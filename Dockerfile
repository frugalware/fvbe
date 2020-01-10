FROM registry.gitlab.com/frugalware/pacman-g2:latest

WORKDIR /tmp/fst
COPY . /tmp/fst
RUN make 
