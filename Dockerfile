FROM frugalware/pacman-g2:latest

WORKDIR /tmp/fst
COPY . /tmp/fst
RUN make 
