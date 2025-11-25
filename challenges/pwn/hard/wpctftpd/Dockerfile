FROM alpine:3.22

EXPOSE 2121

ARG FLAG="WPCTF{test_flag}"
ENV WPCTFTPD_NO_AUTH=1

COPY wpctftpd /bin/

RUN addgroup -S ctf && adduser -S ctf -G ctf

RUN mkdir /ftp
RUN echo "${FLAG}" > /ftp/flag
RUN chmod 000 /ftp/flag
RUN chown -R ctf:ctf /ftp
WORKDIR /ftp

USER ctf

CMD ["/bin/wpctftpd", "-p", "2121"]
