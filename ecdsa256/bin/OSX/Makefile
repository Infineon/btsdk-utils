#makefile

CC=gcc

CFLAGS=-D'fopen_s(pFile,filename,mode)=((*(pFile))=fopen((filename),(mode)))==NULL' \
		-D'sprintf_s(a,b,c,d)=sprintf(a,c,d)' \
		-D'rand_s(a)=rand_r(a)' \
		-D'_snprintf(a,b,c,d)=snprintf(a,b,c,d)'

OBJ_FILES_COMMON := ecc_pp.o \
		multprecision.o \
		sha256.o

OBJ_FILES_GENKEY :=	$(OBJ_FILES_COMMON) \
					ecdsakey.o \

OBJ_FILES_SIGN :=	$(OBJ_FILES_COMMON) \
					ecdsasign.o \

OBJ_FILES_VERIFY :=	$(OBJ_FILES_COMMON) \
					ecdsaverify.o

_HEADER_FILES := ecc_pp.h \
		multprecision.h \
		sha256.h \
		targetver.h \
		types.h

%.o: ../src/common/%.c
	$(CC) -c -o $@ $< $(CFLAGS)  -I$(LOCAL_INCLUDES)

LOCAL_INCLUDES := ../src/include

OUTPUT_FILE := ecdsa_genkey ecdsa_sign ecdsa_verify


ecdsa_genkey : $(OBJ_FILES_GENKEY)
	$(CC) -o $@ $^

ecdsa_sign : $(OBJ_FILES_SIGN)
	$(CC) -o $@ $^

ecdsa_verify : $(OBJ_FILES_VERIFY)
	$(CC) -o $@ $^

all :  ecdsa_genkey ecdsa_sign ecdsa_verify
	@rm *.o

.PHONY: clean

clean :
	@rm *.o $(OUTPUT_FILE) 2> /dev/null
