#define BUFFER_DATE_SIZE 20 + 1 //lunghezza massima della data + '\n'
#define BUFFER_NAME_SIZE 16 + 2 //lunghezza massima del nome + ":\n"
#define BUFFER_IPV4_SIZE 15 //lunghezza massima dell'ipv4
#define BUFFER_MESSAGE 256 + 1 //lunghezza massima del messaggio dell'utente + '\n"
#define BUFFER_SIZE_MESSAGE BUFFER_MESSAGE + BUFFER_NAME_SIZE + BUFFER_DATE_SIZE //lunghezza massima del messaggio incapsulato
