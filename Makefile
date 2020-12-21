NAME 	=	chat 

HEADERS_DIR 	=	 $(shell find . -name "*.h")

SRC 	=	 $(shell find  -name "*.c")

OBJ  	=	$(SRC:.c=.o)

all: 	$(NAME)

$(NAME):	$(OBJ)
	gcc -o $(NAME) $(SRC) $(HEADERS_DIRS) -g -lpthread -D_REENTRANT

temp:
	find . -name "*[#,~,.o]" -type f -delete

clean:   temp
	rm -f $(NAME)
	rm -f $(OBJ)

fclean:  clean
	rm -f $(NAME)

re: 	 fclean  all

auteur:  echo $(USER) > auteur

