#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
//libreria necesaria para poder utilizar el strok
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
//definimos los valores que usaremos como falso y verdadero
#define True 1
#define False 0

//==============================================Area de Variables Globales=========================================
#pragma region Globales
char linea[500]; //Arreglo de char donde se contendra la linea de comando
int comentario=False;//variable que comprueba si es un comentario
int SalirPrograma = False;//Bandera para indicar el fin del programa
int sizeArchivo=0;//indica el tamano
int addParticion=0;//indica el tamano, si se quita o agrega a la particion
int MBR_Size=512;//indica el tamano reservado para el MBR
char *pathArchivo=NULL,*unitArchivo=NULL,*tipoArchivo=NULL,*carpetaArchivo[30];
char *typeParticion=NULL,*deleteParticion=NULL,*fitParticion=NULL,*nameParticion=NULL;
char *idParticion=NULL;
int creada=False;//si se crea la particion logica
int contador_sub=1;//contador de subgrafos
int tamanoLista=0;//contador de tamano de lista principal
char letras[25]= {'a','b','c','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};//numero maximo de particiones activas
int tamano_disco=0;
#pragma endregion Globales

#pragma region Validaciones
//1 = valido, 0=falta parametro
int mkdisk[3]= {False,False,False};//(size,unit,path)
int rmdisk[1]= {False};//(path)
int fdisk[8]= {False,False,False,False,False,False,False,False}; //(size,unit,path,type,fit,delete,name,add)
int mount[2]= {False,False}; //(path,name)
int unmount[1]= {False};//(id)
int executable[1]= {False};
int rep[3]= {False,False,False}; //(id,path,name)

#pragma endregion Validaciones
//=================================================================================================================

//==================================================Area para Structs==============================================
#pragma region Structs
typedef struct
{
    int part_status;
    char part_type;
    char part_fit;
    int part_start;
    int part_size;
    char part_name[16];
} Particion;

typedef struct
{
    int mbr_size;
    char pathActiva[90];
    int mbr_corrupt;
    char mrb_fecha_creacion[128];
    int mbr_disk_signature;
    Particion particion[4];
    int part[4];
    int extend;
} MBR;

typedef struct
{
    int part_status;
    int part_corrupt;
    char part_fit;
    int part_next;
    int part_start;
    int part_size;
    char part_name[16];
} EBR;

#pragma endregion Structs
//=================================================================================================================
#pragma region Estructuras
typedef struct nodoSecundario//estructura para lista secundaria
{
    int numeroParticion;//guardara el numero asignado a la particion
    char *id;//guardara "vb"+letra
    char *idNumero;//guardara "vb"+letra+numero
    char *ruta;
    struct nodoSecundario *siguiente, *anterior;//enlaces para lista secundaria
} NodoSec;

typedef struct nodoPrincipal//estructura para lista principal
{
    int numeroDisco;//numero que le asigna el programa
    char *id;//guarda "vb" + letra
    char *nameDisco;//nombre de disco
    int tamano;//tamano de sublistas
    struct nodoPrincipal *abajo,*arriba;//enlaces para lista principal
    struct nodoSecundario *siguiente,*anterior;//enlaces para lista secundaria
} NodoPrincipal;

NodoPrincipal*insertarPrincipal(NodoPrincipal**p, char* r);
void insertarSecundario(NodoSec **s,char* d,char *part,char *ruta);
NodoPrincipal*crearNodoPrincipal(char* r);
NodoSec*crearNodoSecundario(NodoSec **s, char* d,char *part,char *ruta);
NodoPrincipal*buscarPrincipal(NodoPrincipal*p,char* r);
void imprimir(NodoPrincipal*p);
NodoPrincipal *primero;
char *getRuta(NodoPrincipal **p, char *id);

void insertar(NodoPrincipal ** p, char* row, char* dato)//metodo para insertar un nuevo nodo
{
    NodoPrincipal*principal = insertarPrincipal(p,row);//crea un nodo, lo inserta y lo devuelve
    principal->tamano++;//incrementamos el tamano de la lista secundaria
    insertarSecundario(&(principal->siguiente),dato,principal->id,principal->nameDisco);//insertamos el nodo secundario dentro del principal devuelto
}
void eliminar(NodoPrincipal **p,char *id)//eliminar un nodo por nombre de montura
{
    NodoPrincipal *columna = *p;//tomamos la raiz de la lista
    NodoSec *fila = columna->siguiente;//nos desplazamos hacia la derecha
    while(columna!=NULL)//mientras hayan columnas que recorrer las recorremos
    {
        fila=columna->siguiente;//nos desplazamos a la primera posicion de la lista secundaria
        while(fila!=NULL)//mientras nos podamos desplazar entre filas
        {
            if(strcmp(id,fila->idNumero)==0)//condicional si el nodo con el id
            {
                printf("Se encontro la particion \n");
                printf("Particion a desmontar con id: %s \n",id);
                if(fila->anterior!=NULL)//mientras exista anterior a donde regresar lo tomamos
                    fila->anterior->siguiente=fila->siguiente;//el siguiente del valor anterior sera el siguiente del actual
                else//de lo contrario, hacemos como primero al siguiente
                    columna->siguiente=fila->siguiente;//el actual toma el lugar del primero en lista principal
                columna->tamano--;//restamos el tamano a la lista secundaria
                if(columna->tamano==0) //si no existen nodos en esa lista entonces la eliminamos
                {
                    if(columna->arriba!=NULL)
                        columna->arriba->abajo=columna->abajo;//hacemos que el siguiente del anterior tome el valor del siguiente del actual
                    tamanoLista--;//restamos elementos a lista principal
                }
                if(tamanoLista==0)//si llega a 0 entones no hay elementos
                    primero=NULL;//no existe primerp
                return;
            }
            else
            {
                fila=fila->siguiente;//cambiamos a la siguiente posicion en la lista secundaria
            }
        }
        columna=columna->abajo;//cambiamos a la siguiente posicion en la lista principal
    }
    printf("No se encontro la particion\n");
}

NodoPrincipal*crearNodoPrincipal(char* r)
{
    NodoPrincipal* nuevo = (NodoPrincipal*)malloc(sizeof(NodoPrincipal));
    nuevo->nameDisco=r;
    printf("Ruta disco: %s\n\n",nuevo->nameDisco);
    nuevo->abajo=nuevo->arriba=NULL;
    nuevo->siguiente=nuevo->anterior=NULL;
    nuevo->numeroDisco=asignarNumero();
    char *nombrePart = malloc(30);
    strcpy(nombrePart,"vd");
    stchcat(nombrePart,letras[nuevo->numeroDisco]);
    nuevo->id=nombrePart;
    nuevo->tamano=0;
    return nuevo;
}
NodoSec*crearNodoSecundario(NodoSec **s, char* d,char *part,char *rutaa)
{
    NodoSec* nuevo = (NodoSec*)malloc(sizeof(NodoSec));
    nuevo->id = d;
    nuevo->numeroParticion=asignarParticion(&s);
    char *nombrePart = malloc(30);
    strcpy(nombrePart,part);
    char *nombreRuta=malloc(200);
    strcpy(nombreRuta,rutaa);
    nuevo->ruta=nombreRuta;
    char c = '0'+nuevo->numeroParticion;
    stchcat(nombrePart,c);
    printf("Se monto particion como: %s\n\n",nombrePart);
    nuevo->idNumero=nombrePart;
    nuevo->siguiente = nuevo->anterior = NULL;
    return nuevo;
}
NodoPrincipal* buscarPrincipal(NodoPrincipal* p,char* r)
{
    NodoPrincipal *tmp = p;
    while(tmp!=NULL)
    {
        if(strcmp(tmp->nameDisco,r)==0)
        {
            return tmp;
        }
        tmp=tmp->abajo;
    }
    return NULL;
}

NodoPrincipal *insertarPrincipal(NodoPrincipal**p,char* r)
{


    NodoPrincipal*aux=buscarPrincipal(*p,r);
    if(aux!=NULL)
    {
        return aux;
    }
    else
    {
        NodoPrincipal* nuevo = crearNodoPrincipal(r);
        if(*p!=NULL)
        {
            nuevo->abajo = *p;
            (*p)->arriba=nuevo;
        }
        *p=nuevo;
        tamanoLista++;
        return *p;
    }

}
void insertarSecundario(NodoSec **s, char* d,char *part,char *ruta)
{
    NodoSec * nuevo= crearNodoSecundario((*s),d,part,ruta);
    if(*s!=NULL)
    {
        nuevo->siguiente=*s;
        (*s)->anterior=nuevo;
    }
    *s=nuevo;
}

void imprimir(NodoPrincipal*p)
{
    NodoPrincipal*aux_p =p;

    while(aux_p!=NULL)
    {
        NodoSec *aux_s= aux_p->siguiente;
        printf("+-----+\n| %s |-> ",aux_p->id);
        while(aux_s!=NULL)
        {
            if(aux_s->siguiente!=NULL)
            {
                printf("%s -> ",aux_s->idNumero);
            }
            else
            {
                printf(" %s\n",aux_s->idNumero);
            }
            aux_s=aux_s->siguiente;
        }
        printf("+-----+\n");
        aux_p=aux_p->abajo;
    }
}

int asignarNumero()
{
    NodoPrincipal *tmp = primero;
    int contador=0;
    int salir=0;
    int valido=1;

    while(salir==0)
    {
        valido=1;
        tmp = primero;
        while(tmp!=NULL)
        {
            if(tmp->numeroDisco==contador)
            {
                valido=0;
            }
            tmp=tmp->abajo;
        }
        if(valido==1)
            return contador;
        contador++;
    }

    return 0;
}

int asignarParticion(NodoSec **p)
{
    NodoSec *tmp = *p;
    int contador=1;
    int salir=0;
    int valido=1;

    while(salir==0)
    {
        valido=1;
        tmp = (*p);
        while(tmp!=NULL)
        {
            if(tmp->numeroParticion==contador)
            {
                valido=0;
            }
            tmp=tmp->siguiente;
        }
        if(valido==1)
            return contador;
        contador++;
    }

    return 0;
}

void stchcat(char *cadena, char chr)
{
    size_t longitud = strlen(cadena);

    *(cadena + longitud) = chr;
    *(cadena + longitud + 1) = '\0';
}

char *getRuta(NodoPrincipal **p, char *id)
{
    NodoPrincipal *columna = *p;//tomamos la raiz de la lista
    NodoSec *fila = columna->siguiente;//nos desplazamos hacia la derecha
    while(columna!=NULL)//mientras hayan columnas que recorrer las recorremos
    {
        fila=columna->siguiente;//nos desplazamos a la primera posicion de la lista secundaria
        while(fila!=NULL)//mientras nos podamos desplazar entre filas
        {
            if(strcmp(id,fila->idNumero)==0)//condicional si el nodo con el id
            {
                return fila->ruta;
            }
            else
            {
                fila=fila->siguiente;//cambiamos a la siguiente posicion en la lista secundaria
            }
        }
        columna=columna->abajo;//cambiamos a la siguiente posicion en la lista principal
    }
    printf("No se encontro la particion\n");
    return "NULL";
}


//-================================================================================================================

typedef struct nodo
{
    int indice;
    int numero;
    struct nodo *siguiente, *anterior;
} Nodo;

Nodo* crearNodo(int numero,int indice)
{
    Nodo *nuevo =(Nodo*)malloc(sizeof(Nodo));
    nuevo->numero=numero;
    nuevo->indice=indice;
    nuevo->siguiente=NULL;
    nuevo->anterior=NULL;
    return nuevo;
}

void *insertarl(Nodo **s,int numero, int indice)
{
    Nodo * nuevo= crearNodo(numero,indice);
    //Este algoritmo deben de modificar para poder ordenar
    //la lista
    if(*s!=NULL)//comprobamos si el nodo de entrada no es nulo
    {
        if((*s)->numero<numero)//si el valor actual es menor entonces hacemos otra validacion
        {
            if((*s)->siguiente!=NULL)//si existe algun siguiente
                insertarl(&(*s)->siguiente,numero,indice);//metodo recursivo para insertar hasta el final si es mayor
            else
                (*s)->siguiente=nuevo;//de lo contrario se inserta
        }
        else if((*s)->numero>numero)//si es menor entonces se inserta como primero de fila
        {
            nuevo->siguiente=*s;
            *s=nuevo;
        }
        else if((*s)->numero==numero)
        {
            //si encuentra el mismo dato no lo inserta
        }
        else
        {
            nuevo->siguiente=*s;//para la primera inserccion
        }
    }
    else
        *s=nuevo;//para la primera inserccion
}
#pragma endregion Estructuras
MBR getMBR(char *path);
Particion crearParticion(int inicio);
Particion getExtendida(MBR mbr);
int main()
{
    printf("--------------Primer Proyecto de Manejo e Implementos de Archivos------------\n");
    consola();
    printf("---------------------------USTED ACABA DE SALIR-------------------------------");
    return 0;

}

void consola()
{
    while(SalirPrograma!=True)
    {
        printf("                       --Ingrese el Comando a Realizar--\n");
        scanf("%[^\n]", linea);
        while(getchar()!='\n');
        stchcat(linea,'\n');
        LowerCase(linea);
        Menu(linea);
    }
}

void LowerCase(char cadena[200])//metodo para parsear sin espacios y convertirlo en minusculas
{
    comentario=False;
    char temporal[200];//variable que guardara la palabra de manera temporal
    int contador=0;//contador que se utilizara para guardar la posicion en el temporal
    for(int i=0; i<200; i++)
    {
        //Si viene salto de linea, y retorno de carro
        if(cadena[i]==10||cadena[i]==13||cadena[i]==12)
        {

        }

        else if(cadena[i]==35)
        {
            i++;
            int salir =False;

            while(salir!=True)
            {
                if(cadena[i]=='\n')
                    salir=True;
                i++;
            }
            comentario=True;
        }
        else
        {

            if(cadena[i]==45)
            {
                i++;
                if(cadena[i]==62)//analizamos el siguiente token
                {
                    temporal[contador]='=';//sustituimos el -> por =

                }
                else//de lo contrario restamos al ciclo para que quede en la misma posicion
                {
                    i--;
                    temporal[contador]=tolower(cadena[i]);//hacemos que la cadena va a ser igual al char de entrada
                }
                contador++;

            }
            else
            {
                temporal[contador]=tolower(cadena[i]);
                contador++;
            }

            if(cadena[i]==34) //si vienen comillas verifica si hay espacios adentro y los cambiara por _
            {
                contador--;
                i++;
                while(cadena[i]!=34)
                {
                    if(cadena[i]==32)
                        temporal[contador]='_';
                    else
                    {
                        temporal[contador]=tolower(cadena[i]);
                    }
                    i++;
                    contador++;
                }
            }
            //aumentamos el contador.

        }
    }
    memcpy(&linea,&temporal,sizeof(temporal));//copiamos el valor temporal en linea
}

void Menu(char *linea)
{
    if(strlen(linea)>0)
    {
        char separador[] =" ";
        char *trozo = NULL;//Trozo el cual recoge el modulo que se va a ejecutar
        trozo = strtok(linea, separador);//el primer trozo indica que comando vamos a utilizar
        if(strcmp(trozo, "exit") == 0)
        {
            SalirPrograma=True;
            return;
        }
        else if(strcmp(trozo, "mkdisk") == 0)
        {
            //Imprimimos que esta en el menu de creacion de archivos
            printf("\n*************************************\n");
            printf("* Ingreso a la creacion de Archivos *\n");
            printf("*************************************\n");
            char *parametros[4];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            //llamamos el metodo MKDISK para optener los valores y crear el archivo
            MKDISK(parametros[0],parametros[1],parametros[2],parametros[3]);
            return;

        }
        else if(strcmp(trozo,"rmdisk")==0)
        {
            //Imprimimos que esta en el menu de eliminacion de archivos
            printf("\n****************************************\n");
            printf("* Ingreso a la Eliminacion de Archivos *\n");
            printf("****************************************\n");
            char *parametros[2];//arreglo para guardar los parametros de la creacion de un archivo binario
            for(int i=0; i<2; i++)//for que recorre en busca de - para hacer split
            {
                parametros[i]=trozo; //guardamos el trozo de cadena
                trozo = strtok( NULL, separador); //hacemos el proximo split

            }
            RMDISK(parametros[0],parametros[1]); //llamamos el metodo MKDISK para optener los valores y crear el archivo
            return;

        }
        else if(strcmp(trozo,"fdisk")==0)
        {
            //Imprimimos que esta en el menu de creacion de particiones
            printf("\n***************************************\n");
            printf("* Ingreso a la Modificacion de Discos *\n");
            printf("***************************************\n");
            char *parametros[8];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            //llamamos el metodo MKDISK para optener los valores y crear el archivo
            FDISK(parametros[0],parametros[1],parametros[2],parametros[3],parametros[4],parametros[5],parametros[6],parametros[7]);
            return;

        }
        else if(strcmp(trozo,"exec")==0)
        {
            //Imprimimos que esta en el menu de creacion de particiones
            printf("\n************************************\n");
            printf("* Ingreso a la Carga de Archivos   *\n");
            printf("************************************\n");
            char *parametros[2];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            EXECUTABLE(parametros[0],parametros[1]);
            return;
        }
        else if(strcmp(trozo,"mount")==0)
        {
            //Imprimimos que esta en el menu de creacion de particiones
            printf("\n*********************************\n");
            printf("* Ingreso a Montar Particiones  *\n");
            printf("*********************************\n");
            char *parametros[3];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            MOUNT(parametros[0],parametros[1],parametros[2]);
            return;
        }
        else if(strcmp(trozo,"unmount")==0)
        {
            //Imprimimos que esta en el menu de creacion de particiones
            printf("\n************************************\n");
            printf("* Ingreso a Desmontar Particiones  *\n");
            printf("************************************\n");
            char *parametros[2];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            UNMOUNT(parametros[0],parametros[1]);
            return;
        }
        else if(strcmp(trozo,"rep")==0)
        {
            //Imprimimos que esta en el menu de creacion de particiones
            printf("\n***********************************\n");
            printf("* Ingreso a Creacion de Reportes  *\n");
            printf("***********************************\n");
            char *parametros[4];//arreglo para guardar los parametros de la creacion de un archivo binario
            int i =0;//contador de parametros
            int salir_ciclo=False;//bandera para salir de ciclo
            while(salir_ciclo!=True)
            {
                parametros[i]=trozo;//array de parametros
                trozo = strtok( NULL, separador);//hacemos el proximo split
                i++;
                if(trozo==NULL)
                    salir_ciclo=True;
                else
                    salir_ciclo=False;
            }
            REP(parametros[0],parametros[1],parametros[2],parametros[3]);
            return;
        }
    }
}
//================================================Creacion de Discos=============================================
#pragma region MKDISK

void MKDISK(char *cmd,char *size, char *unit, char *path)//Metodo para armar el comando MKDISK
{
    //Arreglo para verificar los parametros
    mkdisk[0]=False;
    mkdisk[1]=False;
    mkdisk[2]=False;

    split(size,cmd);//separamos la parte de tamano y obtenemos su valor
    split(unit,cmd);//separamos la parte de unidad y obtenemos su valor
    split(path,cmd);//Separamos la parte de ruta y obtenemos su valor
    int valido = validar(cmd);
    if(valido==True)
    {
        printf("Tamano de la unidad a crear: %d\n",sizeArchivo);
        printf("Tipo de unidad: %s\n",tipoArchivo);
        printf("Ruta de archivo a crear: %s\n",pathArchivo);
        separarRuta(pathArchivo);//separamos la ruta para poder crear el arbol de carpetas donde se creeara el archivo
        if(sizeArchivo>0)
            CrearArchivo(sizeArchivo,unitArchivo,pathArchivo);//creamos el archivo binario que sera nuestro disco duro virtual
        else
            printf("No se puede crear archivo por que es 0 o numero negativo\n");

    }
}

void CrearArchivo(int size, char *unit,char *path)//metodo interno para creacion de archivos
{
    char comando[500];
    strcpy(comando,"sudo mkdir -p ");//comando para crear arbol de carpetas
    strcat(comando,carpetaArchivo);//concatenamos el comando con la ruta de carpetas
    system(comando);//ejecutamos el comando
    strcpy(comando,"sudo chmod 777 ");//comando para dar permisos de escritura lectura y ejecucion
    strcat(comando,carpetaArchivo);//concatenamos el comando con la carpeta a dar permisos
    system(comando);//ejecutamos el comando
    int bt=size;
    int bytes = bt*8; //bytes a poner en el archivo
    int kilobytes = bt*1024;//kilobytes en el archivo
    int megabytes =bt*1024*1024;//megabytes en el archivo
    char *bite='\0';
    FILE *archivo = fopen(path,"wb+");//creamos un archivo tipo solo para abrir
    if(archivo)//comprobamos si existe el archivo
    {
        if(strcmp(unitArchivo,"b")==0)//comparamos si es bytes
        {
            for(int i=0; i<bytes; i++) //ciclo que llena el archivo de 0 byte por byte
            {
                fwrite(&bite,1,1,archivo);
            }
        }
        else if(strcmp(unitArchivo,"k")==0)//comparacion si es kilobytes
        {
            for(int i=0; i<kilobytes; i++)
            {
                fwrite(&bite,1,1,archivo);
            }
        }
        else if (strcasecmp(unitArchivo,"m")==0)//comparamos si es megabyte
        {
            for(int i=0; i<megabytes; i++)
            {
                fwrite(&bite,1,1,archivo);
            }
        }
        else
        {
            printf("=====NO SE PUDO CREAR EL DISCO===\n");//error no ejecutar

        }
        printf("Se creo el archivo en el directorio\n");
    }
    else
    {
        printf("No se pudo crear el Archivo\n");
    }
    fclose(archivo);
    printf("********************* Proceso Terminado ************************\n");
    printf("\n");

}
#pragma endregion MKDISK
//=============================================Fin Creacion de Discos================================================
//*******************************************************************************************************************
//===============================================Eliminacion de Discos===============================================

#pragma region RMDISK

void RMDISK(char *cmd,char *path)
{
    rmdisk[0]=False;
    split(path,cmd);//separamos el ruta para obtener el valor

    if(rmdisk[0]==True)
    {
        printf("Ruta donde se va a eliminar: %s\n",pathArchivo);
        char comando[500];
        FILE *archivo = fopen(pathArchivo,"rb");
        if(archivo)
        {
            fclose(archivo);

            strcpy(comando,"sudo rm ");
            strcat(comando,pathArchivo);
            system(comando);
            archivo=fopen(pathArchivo,"rb");
            if(archivo)
            {
                printf("No se pudo eliminar el Disco\n");
            }
            else
            {
                printf("Disco Eliminado con exito\n");
            }
        }

        else
        {
            printf("No existe Disco en la direccion %s\n",pathArchivo);
        }
        pathArchivo=NULL;
    }
    else
    {
        printf("No ha ingresado una ruta\n");
    }
    printf("********************* Proceso Terminado ************************\n");
    printf("\n");

}

#pragma endregion RMDISK
//=============================================Fin Eliminacion de Discos=============================================
//*******************************************************************************************************************
//===============================================Particionar de Discos===============================================
#pragma region FDISK
void FDISK(char *cmd,char *size, char *unit, char *path,char *type,char *fit,char *del, char *name, char *add)
{
    //Banderas necesarias para validar parametros.
    fdisk[0]=False;
    fdisk[1]=False;
    fdisk[2]=False;
    fdisk[3]=False;
    fdisk[4]=False;
    fdisk[5]=False;
    fdisk[6]=False;
    fdisk[7]=False;

    sizeArchivo=0;
    split(size,cmd);//separamos la parte de tamano y obtenemos su valor
    split(unit,cmd);//separamos la parte de unidad y obtenemos su valor
    split(path,cmd);//Separamos la parte de ruta y obtenemos su valor
    split(type,cmd);
    split(fit,cmd);
    split(del,cmd);
    split(name,cmd);
    //split(add,cmd);

    int valido=validar(cmd);

    if(valido==True)
    {
        int existe_MBR = existeMBR(pathArchivo);
        if(existe_MBR==0)
        {
            printf("        No existe MBR en el disco se creara.\n");
            crearMBR(pathArchivo);
            printf("        MBR se creo con exito, se procedera a crear particion\n\n");
        }
        else
            printf("        MBR encontrado, se procedera a crear la particion\n\n");


        int salir_busqueda=False;
        int espacio_particion =False;
        int inicio_particion=513;
        MBR mbrAuxiliar;
        mbrAuxiliar = getMBR(pathArchivo);
        int contador_particion=0;
        for(int i =0; i<4; i++)
        {
            if(mbrAuxiliar.part[i]==False)
                contador_particion++;
        }

        if(fdisk[7]==False&&fdisk[5]==False)//Si no trae el parametro de eliminar y agregar
        {

            if(mbrAuxiliar.extend==False)
                printf("(Quedan %d Primarias, 1 Extendida)\n",(contador_particion-1));
            else
                printf("(Quedan %d Primarias, 0 Extendida)\n",(contador_particion));

            if(contador_particion>=1)//si existe espacio en la particion
            {
                if(sizeArchivo>0)
                {

                    printf("----------------------Atributos de Particion------------------------------\n");
                    printf("Tamano de Particion: %d\n",sizeArchivo);
                    printf("Unidad de Particion: %s\n",tipoArchivo);
                    printf("Ruta fisica de Disco: %s\n",pathArchivo);
                    printf("Tipo de Particion: %s\n",typeParticion);
                    printf("Tipo de Ajuste: %s\n",fitParticion);
                    printf("Nombre de Particion: %s\n",nameParticion);
                    printf("--------------------------------------------------------------------------\n");
                    tamano_disco=mbrAuxiliar.mbr_size;
                    if(strcmp(typeParticion,"e")==0&&mbrAuxiliar.extend==False)//si viene una particion e y no existe particion
                    {
                        int indice=0;
                        while(salir_busqueda==False)
                        {
                            if(mbrAuxiliar.part[indice]==False)
                            {
                                inicio_particion=PrimerAjuste(mbrAuxiliar);
                                if(inicio_particion!=-1)
                                {
                                    mbrAuxiliar.extend=True;
                                    mbrAuxiliar.part[indice]=True;
                                    mbrAuxiliar.particion[indice]=crearParticion(inicio_particion);
                                    FILE *archivo = fopen(pathArchivo,"rb+");
                                    fseek(archivo,0,SEEK_CUR);
                                    fwrite(&mbrAuxiliar,sizeof(MBR),1,archivo);
                                    fclose(archivo);
                                    printf("-------------------Particion Extendida Creada con Exito-------------------\n");
                                    printf("--------------------------------------------------------------------------\n\n");
                                }
                                else
                                    printf("--------Espacio insuficiente para crear particion ---------\n\n");

                                return;
                            }
                            indice++;
                        }

                        //codigo para crear una particion extendida
                    }
                    //else
                    // printf("No se pueden crear mas particiones extendidas\n");

                    else if(strcmp(typeParticion,"p")==0)
                    {
                        int indice=0;
                        while(salir_busqueda==False)
                        {
                            if(mbrAuxiliar.part[indice]==False)
                            {
                                inicio_particion=PrimerAjuste(mbrAuxiliar);
                                if(inicio_particion!=-1)
                                {
                                    mbrAuxiliar.part[indice]=True;
                                    mbrAuxiliar.particion[indice]=crearParticion(inicio_particion);
                                    FILE *archivo = fopen(pathArchivo,"rb+");
                                    fseek(archivo,0,SEEK_CUR);
                                    fwrite(&mbrAuxiliar,sizeof(MBR),1,archivo);
                                    fclose(archivo);
                                    printf("-------------------Particion Primaria Creada con Exito--------------------\n");
                                    printf("--------------------------------------------------------------------------\n\n");

                                }
                                else
                                    printf("--------Espacio insuficiente para crear particion ---------\n\n");
                                return;
                            }
                            indice++;
                        }
                    }//else
                    // printf("No se pueden crear mas particiones primarias\n");

                    else if(strcmp(typeParticion,"l")==0&&mbrAuxiliar.extend==True)
                    {
                        printf("----------------------Atributos de Particion------------------------------\n");
                        printf("Tamano de Particion: %d\n",sizeArchivo);
                        printf("Unidad de Particion: %s\n",tipoArchivo);
                        printf("Ruta fisica de Disco: %s\n",pathArchivo);
                        printf("Tipo de Particion: %s\n",typeParticion);
                        printf("Tipo de Ajuste: %s\n",fitParticion);
                        printf("Nombre de Particion: %s\n",nameParticion);
                        printf("--------------------------------------------------------------------------\n");
                        MBR aux = getMBR(pathArchivo);
                        Particion extendida = getExtendida(aux);//obtenemos el mbr luego la particion extendida
                        creada=False;
                        crearLogica(extendida.part_start,(extendida.part_start+extendida.part_size));
                    }
                    else
                    {
                        printf("******************* No se pueden crear mas particiones *******************\n\n");
                    }

                }
                else
                    printf("No se puede crear particion por que es 0 o numero negativo\n");
            }
            else if(strcmp(typeParticion,"l")==0&&mbrAuxiliar.extend==True) //si la particion es logica
            {
                printf("----------------------Atributos de Particion------------------------------\n");
                printf("Tamano de Particion: %d\n",sizeArchivo);
                printf("Unidad de Particion: %s\n",tipoArchivo);
                printf("Ruta fisica de Disco: %s\n",pathArchivo);
                printf("Tipo de Particion: %s\n",typeParticion);
                printf("Tipo de Ajuste: %s\n",fitParticion);
                printf("Nombre de Particion: %s\n",nameParticion);
                printf("--------------------------------------------------------------------------\n");
                MBR aux = getMBR(pathArchivo);
                Particion extendida = getExtendida(aux);//obtenemos el mbr luego la particion extendida
                creada=False;
                crearLogica(extendida.part_start,(extendida.part_start+extendida.part_size));

            }
            else
                printf("******************* No se pueden crear mas particiones *******************\n\n");

        }
        else if(fdisk[7]==True)//si encuentra el parametro add
        {

        }
        else if(fdisk[5]==True)//si encuentra el parametro delete
        {

        }
    }
}
int PrimerAjuste(MBR mbrActual)
{
    int sizeParticion=0;
    int bt=sizeArchivo;
    int bytes = bt*8; //bytes a poner en el archivo
    int kilobytes = bt*1024;//kilobytes en el archivo
    int megabytes =kilobytes*1024;//megabytes en el archivo
    int fin =tamano_disco;

    if(strcmp(unitArchivo,"m")==0)
    {
        sizeParticion=megabytes;
    }
    else if(strcmp(unitArchivo,"k")==0)
    {
        sizeParticion=kilobytes;
    }
    else if(strcmp(unitArchivo,"b")==0)
    {
        sizeParticion=bytes;
    }


    int restante=0;
    int final_=0;

    int indice=0;



    Nodo *primero=NULL;
    while(indice<4)
    {
        if(mbrActual.part[indice]==True)//comprobamos si hay particiones
        {
            insertarl(&primero,mbrActual.particion[indice].part_start,indice);//insertamos el numero de la particion en la lista ordenada
        }
        indice++;
    }

    if(primero==NULL)//si no encontro ninguna particion
        return 512;//retornamos para que lo inserte al principio
    else
    {
        while(primero!=NULL) //recorremos la lista para saber donde hay espacio disponible
        {
            if(primero->siguiente!=NULL) //si el siguiente no es nulo lo comparamos con el siguiente indice
            {
                final_=mbrActual.particion[primero->indice].part_start+mbrActual.particion[primero->indice].part_size;
                restante = mbrActual.particion[primero->siguiente->indice].part_start-final_;
                if(restante>0)
                    return final_;
            }
            else//de lo contrario lo comparamos con el final del archivo
            {
                restante=fin - (mbrActual.particion[primero->indice].part_start+mbrActual.particion[primero->indice].part_size);
                if(restante>=0)
                    return mbrActual.particion[primero->indice].part_start+mbrActual.particion[primero->indice].part_size;
            }


            primero=primero->siguiente;
        }
    }
    printf("---*---No existe espacio para poder crear particion---*---\n");
    return -1;
}

Particion crearParticion(int inicio)
{
    int bt=sizeArchivo;
    int bytes = bt*8; //bytes a poner en el archivo
    int kilobytes = bt*1024;//kilobytes en el archivo
    int megabytes =kilobytes*1024;//megabytes en el archivo

    int tamano_restante=tamano_disco-inicio;
    Particion part;
    part.part_start=inicio;
    part.part_status=True;
    if(strcmp(fitParticion,"bf")==0)
        part.part_fit='b';
    else if(strcmp(fitParticion,"wf")==0)
        part.part_fit='w';
    else if(strcmp(fitParticion,"ff")==0)
        part.part_fit='f';
    if(strcmp(typeParticion,"p")==0)
        part.part_type='p';
    else if(strcmp(typeParticion,"e")==0)
        part.part_type='e';

    strcpy(part.part_name,nameParticion);
    if(strcmp(unitArchivo,"m")==0)
        part.part_size=megabytes;
    else if(strcmp(unitArchivo,"k")==0)
        part.part_size=kilobytes;
    else if(strcmp(unitArchivo,"b")==0)
        part.part_size=bytes;

    return part;


}

MBR getMBR(char *path)
{
    FILE *archivo = fopen(path,"rb+");
    MBR mbrActual;
    if(archivo!=NULL)
    {

        fseek(archivo,0,SEEK_CUR);//posicionamos el puntero al principio del archivo
        fread(&mbrActual,sizeof(MBR),1,archivo);//leemos el mbr
        fclose(archivo);
    }
    return mbrActual;
}

int existeMBR(char *path)
{
    int valor=0;

    FILE *archivo = fopen(path,"rb");
    if(archivo)
    {
        fseek(archivo,0,SEEK_CUR);
        MBR mbrPueba;
        fread(&mbrPueba,sizeof(MBR),1,archivo);
        if(mbrPueba.mbr_corrupt!=0)
        {
            valor =1;
        }
        fclose(archivo);
    }
    return valor;
}

void crearMBR(char *ruta)
{
    printf("Ruta donde se creara: %s \n",ruta);

    FILE *archivo = fopen(ruta,"r");
    if(archivo!=NULL)
    {
        fseek(archivo, 0, SEEK_END);
        tamano_disco=ftell(archivo);
        fclose(archivo);
    }

    archivo = fopen(ruta,"rb+");
    if(archivo)
    {
        MBR mbr;
        mbr.mbr_corrupt=4;
        mbr.mbr_size=tamano_disco;
        //printf("tamano que ocupa %d bytes\n", mbr.mbr_size);
        strcpy(mbr.pathActiva,ruta);
        time_t tiempo = time(0);
        mbr.part[0]=False;
        mbr.part[1]=False;
        mbr.part[2]=False;
        mbr.part[3]=False;
        mbr.extend=False;
        struct tm *tlocal = localtime(&tiempo);
        strftime(mbr.mrb_fecha_creacion,128,"%d/%m/%y %H:%M:%S",tlocal);
        fseek(archivo,0,SEEK_CUR);
        fwrite(&mbr,sizeof(MBR),1,archivo);
        printf("Se escribio el mbr correctamente en los primeros 512 bytes del disco\n\n");
        fclose(archivo);


    }
    else
    {
        printf("No existe disco para crear MBR\n\n");
    }
}

Particion getExtendida(MBR mbr) //metodo que traera la particion extendida;
{
    int i=0;
    int salir= False;
    while(salir!=True)
    {
        if(mbr.part[i]==1)
        {
            if(mbr.particion[i].part_type=='e')
                return mbr.particion[i];
        }
        i++;
    }
    Particion part;
    return part;

}


void crearLogica(int inicio,int tamTotal)
{
    int bt=sizeArchivo;
    int bytes = bt*8; //bytes a poner en el archivo
    int kilobytes = bt*1024;//kilobytes en el archivo
    int megabytes =kilobytes*1024;//megabytes en el archivo

    int tamano_restante=tamano_disco-inicio;

    EBR ebr;
    FILE *archivo =fopen(pathArchivo,"rb+");
    if(archivo)
    {
        fseek(archivo,inicio,SEEK_SET);
        fread(&ebr,sizeof(EBR),1,archivo);
        fclose(archivo);
        if(ebr.part_next==0)
        {
            EBR ebr_nuevo;
            ebr_nuevo.part_corrupt=4;
            if(strcmp(fitParticion,"bf")==0)
                ebr_nuevo.part_fit='b';
            else if(strcmp(fitParticion,"wf")==0)
                ebr_nuevo.part_fit='w';
            else if(strcmp(fitParticion,"ff")==0)
                ebr_nuevo.part_fit='f';

            ebr_nuevo.part_next=-1;
            strcpy(ebr_nuevo.part_name,nameParticion);

            if(strcmp(unitArchivo,"m")==0)
                ebr_nuevo.part_size=megabytes;
            else if(strcmp(unitArchivo,"k")==0)
                ebr_nuevo.part_size=kilobytes;
            else if(strcmp(unitArchivo,"b")==0)
                ebr_nuevo.part_size=bytes;
            ebr_nuevo.part_start=inicio;
            ebr_nuevo.part_status=0;
            if(tamTotal-(inicio+ebr_nuevo.part_size)>=0)
            {
                FILE *temp =fopen(pathArchivo,"rb+");
                fseek(temp,inicio,SEEK_SET);
                fwrite(&ebr_nuevo,sizeof(EBR),1,temp);
                fclose(temp);
                creada=True;
                printf("--------------------Particion Logica Creada con Exito---------------------\n");
                printf("--------------------------------------------------------------------------\n\n");
                return;
            }
            else
            {
                creada=False;
                printf("----------Se necesita mas espacio para poder insertar particion-----------\n");
                printf("--------------------------------------------------------------------------\n\n");
            }
        }
        else if(ebr.part_next==-1)
        {
            crearLogica((ebr.part_start+ebr.part_size),tamTotal);

            if(creada!=False)
            {
                ebr.part_next=(ebr.part_start+ebr.part_size);

                archivo =fopen(pathArchivo,"rb+");
                fseek(archivo,inicio,SEEK_CUR);
                fwrite(&ebr,sizeof(EBR),1,archivo);
                fclose(archivo);

            }
        }
        else
        {
            crearLogica(ebr.part_next,tamTotal);
        }
    }

}
#pragma endregion FDISK
int contadorprueba=0;
//=============================================Fin Particionar de Discos=============================================
#pragma region MOUNT
void MOUNT(char *cmd, char *path, char *name)
{
    mount[0]=False;
    mount[1]=False;
    contadorprueba++;

    split(path,cmd);
    split(name,cmd);


    if(mount[0]==True&&mount[1]==True)
    {

        if(existeParticion(pathArchivo,nameParticion)==True)
        {
            char *aux=malloc(30);
            char *aux2=malloc(30);

            strcpy(aux,pathArchivo);
            strcpy(aux2,nameParticion);

            insertar(&primero,aux,aux2);
            printf("------------------Vista de Particiones Montadas---------------\n");
            imprimir(primero);
        }
        else
        {
            printf("El directorio %s no contiene ninguna particion llamada %s \n\n",pathArchivo,nameParticion);
        }
    }
    else
    {
        printf("------------No se puede montar la particion-----------\n");
    }

}
int existeParticion(char *path,char *nombre)
{
    FILE *archivo = fopen(path,"rb");
    if (archivo != NULL)
    {
        MBR mbrActual;
        fseek(archivo,0,SEEK_CUR);//posicionamos el puntero al principio del archivo
        fread(&mbrActual,sizeof(MBR),1,archivo);//leemos el mbr
        fclose(archivo);

        for(int i=0; i<4; i++)
        {
            if(mbrActual.part[i]==True)
            {
                if(strcmp(mbrActual.particion[i].part_name,nombre)==0)
                    return True;
                if(mbrActual.particion[i].part_type=='e')
                {
                    return existeLogica(mbrActual.particion[i].part_start);
                }
            }
        }
    }
    return False;
}

int existeLogica(int inicio)
{

    EBR ebr;
    FILE *archivo =fopen(pathArchivo,"rb+");
    if(archivo)
    {
        fseek(archivo,inicio,SEEK_SET);
        fread(&ebr,sizeof(EBR),1,archivo);
        fclose(archivo);
        if(ebr.part_next==0)
            return False;
        else if(ebr.part_next==-1)
        {
            if(strcmp(ebr.part_name,nameParticion)==0)
                return True;
            else
                return False;
        }
        else
        {
            if(strcmp(ebr.part_name,nameParticion)==0)
                return True;
            else
                return existeLogica(ebr.part_next);
        }
    }
    else
    {
        return False;
    }



}

#pragma endregion MOUNT
//================================================================================================================================
#pragma region UNMOUNT
void UNMOUNT(char *cmd, char *id)
{
    unmount[0]=False;

    split(id,cmd);
    if(unmount[0])
    {
        eliminar(&primero,idParticion);
        printf("------------------Vista de Particiones Montadas---------------\n");
        imprimir(primero);
    }
    else
    {
        printf("------------No se puede montar la particion-----------\n");
    }
}

#pragma endregion UNMOUNT
//================================================================================================================================
#pragma region EXECUTABLE
void EXECUTABLE(char *cmd, char*ruta)
{
    executable[0]=False;
    split(ruta,cmd);

    FILE *entrada;
    if(executable[0]==True)
    {

        if ((entrada = fopen(pathArchivo, "r")) == NULL)
        {
            perror(pathArchivo);
            return EXIT_FAILURE;
        }

        while (fgets(linea, 500, entrada) != NULL)
        {
            printf("%s\n",linea);
            stchcat(linea,'\n');
            LowerCase(linea);
            Menu(linea);
        }


        fclose(entrada);
    }
    else
        printf("No se puede abrir archivo por falta de parametros\n");
    printf("********************* Proceso Terminado ************************\n");
    printf("\n");
}
#pragma endregion EXECUTABLE

int validar(char *cmd)
{

    if(strcmp(cmd,"mkdisk")==0)
    {
        if(mkdisk[0]==True && mkdisk[2]==True)
        {
            if(mkdisk[1]==False)
            {
                tipoArchivo="Megabyte";
                unitArchivo="m";
            }
            return True;
        }
    }
    else if(strcmp(cmd,"fdisk")==0)
    {
        if(fdisk[0]==True && fdisk[2]==True&&fdisk[6]==True)
        {
            if(fdisk[1]==False)
            {
                tipoArchivo="Megabyte";
                unitArchivo="m";
            }

            if(fdisk[3]==False)
                typeParticion="p";

            if(fdisk[4]==False)
                fitParticion="wf";

            return True;

        }
        else if(fdisk[0]==True && fdisk[2]==True&&fdisk[6]==True)
        {
            if(fdisk[5]==True)
            {
                return True;
            }
        }
    }
    return False;

}

void split(char *valor,char *cmd)
{
    char *temporal;
    char separador[]="=";
    temporal = strtok(valor,separador);//parametro a splitear
    if(temporal!=NULL)
    {
        if(strcmp(temporal,"-size")==0)
        {
            temporal = strtok(NULL,separador); //separamos los valores size del valor numerico
            sizeArchivo = atoi(temporal);//Parseamos el valor a int
            mkdisk[0]=True;//activamos la bandera que si existe el parametro
            fdisk[0]=True;//encontro el parametro en fdisk
        }
        else if(strcmp(temporal,"-unit")==0)
        {
            temporal = strtok(NULL,separador);
            unitArchivo = temporal; //Especificamos que tipo es
            if(strcmp(unitArchivo,"k")==0)
                tipoArchivo="KiloByte";
            else if(strcmp(unitArchivo,"m")==0)
                tipoArchivo="MegaByte";
            else
                tipoArchivo="Byte";
            mkdisk[1]=True;//activamos la bandera que si existe el parametro
            fdisk[1]=True;//encontro el parametro en fdisk
        }
        else if(strcmp(temporal,"-path")==0)
        {
            temporal = strtok(NULL,separador);
            pathArchivo = temporal;
            mkdisk[2]=True;//activamos la bandera que si existe el parametro
            rmdisk[0]=True;//bandera que activa remover el archivo
            fdisk[2]=True;//encontro el parametro en fdisk
            executable[0]=True;
            mount[0]=True;
            rep[1]=True;
        }
        else if(strcmp(temporal,"-type")==0)
        {
            temporal=strtok(NULL,separador);
            typeParticion=temporal;
            fdisk[3]=True;
        }
        else if(strcmp(temporal,"-fit")==0)
        {
            temporal=strtok(NULL,separador);
            fitParticion=temporal;
            fdisk[4]=True;
        }
        else if(strcmp(temporal,"-delete")==0)
        {
            temporal=strtok(NULL,separador);
            deleteParticion=temporal;
            fdisk[5]=True;
        }
        else if(strcmp(temporal,"-name")==0)
        {
            temporal=strtok(NULL,separador);
            nameParticion=temporal;
            fdisk[6]=True;
            mount[1]=True;
            rep[2]=True;
        }
        else if(strcmp(temporal,"-add")==0)
        {
            temporal=strtok(NULL,separador);
            addParticion=atoi(temporal);
            fdisk[7]=True;
        }
        else if(strcmp(temporal,"-id")==0)
        {
            temporal=strtok(NULL,separador);
            idParticion=temporal;
            unmount[0]=True;
            rep[0]=True;
        }
        else
        {
            printf("ERROR---No existe parametro %s en linea de comando %s\n",temporal,cmd);
            return;
        }

        return;
    }
    else
    {
        printf("ERROR---Falta de argumentos en linea de comando %s\n",cmd);
        printf("*****Se tomaran los valores por defecto******\n\n");
    }
    return;
}
void separarRuta(char *ruta)
{

    char palabra[200];//arreglo de char, tomandolo como STRING
    char *string=ruta;//string que toma la ruta como apuntador para no tocar el original
    strcpy(palabra,string); //copiamos el contenido del string de entrada y lo pasamos a variable string
    char *rutatemp;  //variable en donde se alojara la ruta temporal, los nombres de direccion
    rutatemp = strtok(palabra,"/"); //split para separar la ruta temporal
    int contador =0;//contador para que vaya contando cuantas carpetas existen antes del archivo
    //ciclo que recorre el string y lo separa
    while(rutatemp!=NULL)
    {
        rutatemp = strtok(NULL,"/");
        contador++;
    }

    strcpy(palabra,string); //volvemos a copiar el string para divirlo
    rutatemp = strtok(palabra,"/"); //lo separamos por /
    char *rutaDef[30];  //directorio donde se va a juntar la ruta completa de carpetas
    rutaDef[0]='/';
    for(int i=0; i<contador*2-3; i++) //ciclo que concatena el nombre y un / antes de cada nombre
    {
        if(i%2==0)
        {
            strcat(rutaDef,rutatemp);
            rutatemp = strtok(NULL,"/");
        }
        else
        {
            strcat(rutaDef,"/");
        }
    }
    strcpy(carpetaArchivo,rutaDef); //copiamos el directorio de carpetas a la variable global

}

//=================================================================================================================================
#pragma region REP
void REP(char *cmd, char* id, char *path, char *name)
{
    rep[0]=False;
    rep[1]=False;
    rep[2]=False;

    split(id,cmd);
    split(path,cmd);
    split(name,cmd);

    separarRuta(pathArchivo);

    if(rep[0]==True&&rep[1]==True&&rep[2]==True)
    {
        char *ruta = malloc(200);
        ruta=getRuta(&primero,idParticion);
        if(strcmp(ruta,"NULL")!=0)
        {
            printf("***********************************************************\n");
            printf("*RUTA DE DISCO: %s\n",ruta);
            printf("*TIPO DE REPORTE: %s\n",nameParticion);
            printf("*RUTA DE DESTINO: %s\n",carpetaArchivo);
            printf("*ARCHIVO ALOJADO EN: %s\n",pathArchivo);
            printf("***********************************************************\n");

            char comando[500];
            strcpy(comando,"sudo mkdir -p ");//comando para crear arbol de carpetas
            strcat(comando,carpetaArchivo);//concatenamos el comando con la ruta de carpetas
            system(comando);//ejecutamos el comando
            strcpy(comando,"sudo chmod 777 ");//comando para dar permisos de escritura lectura y ejecucion
            strcat(comando,carpetaArchivo);//concatenamos el comando con la carpeta a dar permisos
            system(comando);//ejecutamos el comando
            MBR mbrAux =getMBR(ruta);//obtenemos el mbr del disco

            if(strcmp(nameParticion,"mbr")==0)
            {
                graficarMBR(mbrAux,ruta);

            }
            else if(strcmp(nameParticion,"disk")==0)
            {
                graficarDISCO(mbrAux,ruta);
            }

            else
            {
                printf("No existe %s en los comandos permitidos \n",nameParticion);
                return;
            }
            printf("Grafica creada en %s\n",carpetaArchivo);



        }

    }
}
void graficarMBR(MBR mbr,char *ruta)
{
    char line[200];
    char append[50];
    FILE *archivo=fopen(pathArchivo,"w+");
    if(archivo)
    {
        //filas es tr columnas es td
        fputs("<html>",archivo);
        fputs("<body>",archivo);
        fputs("<h1>MBR</h1>",archivo);
        fputs("<table border=\"1\">",archivo);
        fputs("<tr><td>Nombre</td><td>Valor</td></tr>",archivo);

        fputs("<tr>",archivo);
        fputs("<td>Fecha Creacion",archivo);
        fputs("</td>",archivo);
        fputs("<td>",archivo);
        fputs(mbr.mrb_fecha_creacion,archivo);
        fputs("</td>",archivo);
        fputs("</tr>",archivo);

        fputs("<tr>",archivo);
        fputs("<td>Tamano",archivo);
        fputs("</td>",archivo);
        fputs("<td>",archivo);
        sprintf(append,"%d",mbr.mbr_size); // put the int into a string
        strcpy(line,append);
        fputs(line,archivo);
        fputs("</td>",archivo);
        fputs("</tr>",archivo);

        fputs("<tr>",archivo);
        fputs("<td>Corrupto",archivo);
        fputs("</td>",archivo);
        fputs("<td>",archivo);
        sprintf(append,"%d",mbr.mbr_corrupt); // put the int into a string
        strcpy(line,append);
        fputs(line,archivo);
        fputs("</td>",archivo);
        fputs("</tr>",archivo);
        for(int i=0; i<4; i++)
        {
            if(mbr.part[i]==True)
                insertarenHTMl(archivo,mbr.particion[i]);
        }
        fputs("</table>",archivo);

        for(int i=0; i<4; i++)
        {
            if(mbr.part[i]==True)
                if(mbr.particion[i].part_type=='e')
                    imprimirLogica(archivo,mbr.particion[i].part_start,ruta);
        }
        fputs("</body>",archivo);
        fputs("</html>",archivo);
        fclose(archivo);
    }
    strcpy(line,"firefox ");
    strcat(line,pathArchivo);
    system(line);
}

void insertarenHTMl(FILE *archivo, Particion actual)
{
    char append[50];
    char line[100];
    fputs("<tr>",archivo);
    fputs("<td>Nombre Particion",archivo);
    fputs("</td>",archivo);
    fputs("<td>",archivo);
    fputs(actual.part_name,archivo);
    fputs("</td>",archivo);
    fputs("</tr>",archivo);

    fputs("<tr>",archivo);
    fputs("<td>Tipo",archivo);
    fputs("</td>",archivo);
    fputs("<td>",archivo);
    if(actual.part_type=='p')
        fputs("Primaria",archivo);
    else
        fputs("Extendida",archivo);
    fputs("</td>",archivo);
    fputs("</tr>",archivo);

    fputs("<tr>",archivo);
    fputs("<td>Ajuste",archivo);
    fputs("</td>",archivo);
    fputs("<td>",archivo);
    if(actual.part_type=='b')
        fputs("Mejor Ajuste",archivo);
    else if(actual.part_type=='f')
        fputs("Primer Ajuste",archivo);
    else
        fputs("Peor Ajuste",archivo);
    fputs("</td>",archivo);
    fputs("</tr>",archivo);

    fputs("<tr>",archivo);
    fputs("<td>Inicio Part",archivo);
    fputs("</td>",archivo);
    fputs("<td>",archivo);
    sprintf(append,"%d",actual.part_start); // put the int into a string
    strcpy(line,append);
    fputs(line,archivo);
    fputs("</td>",archivo);
    fputs("</tr>",archivo);

    fputs("<tr>",archivo);
    fputs("<td>Tamano Part",archivo);
    fputs("</td>",archivo);
    fputs("<td>",archivo);
    sprintf(append,"%d",actual.part_size); // put the int into a string
    strcpy(line,append);
    fputs(line,archivo);
    fputs("</td>",archivo);
    fputs("</tr>",archivo);

}
void imprimirLogica(FILE *temp, int inicio,char *ruta)
{
    EBR ebr;
    FILE *archivo =fopen(ruta,"rb+");

    if(archivo)
    {
        fseek(archivo,inicio,SEEK_SET);
        fread(&ebr,sizeof(EBR),1,archivo);
        fclose(archivo);
        printf("NOMBRE DE PARTICION: %s\n",ebr.part_name);
        if(ebr.part_next==0)
        {

        }
        else if(ebr.part_next==-1)
        {
            logicaenHTML(temp,ebr.part_start,ruta);
            imprimirLogica(temp,(ebr.part_start+ebr.part_size),ruta);
        }
        else
        {
            logicaenHTML(temp,ebr.part_start,ruta);
            imprimirLogica(temp,ebr.part_next,ruta);
        }
    }
}



void logicaenHTML(FILE *archivo,int inicioPart,char *ruta)
{

    char append[50];
    char line[200];
    FILE *temp =fopen(ruta,"rb+");
    if(temp)
    {
        EBR ebr;
        fseek(temp,inicioPart,SEEK_CUR);
        fread(&ebr,sizeof(EBR),1,temp);
        if(ebr.part_corrupt==4)
        {
            fputs("<h2>EBR</h2>",archivo);
            fputs("<table border=\"1\">",archivo);
            fputs("<tr><td>Nombre</td><td>Valor</td></tr>",archivo);
            fputs("<tr>",archivo);
            fputs("<td>Nombre",archivo);
            fputs("</td>",archivo);
            fputs("<td>",archivo);
            fputs(ebr.part_name,archivo);
            fputs("</td>",archivo);
            fputs("</tr>",archivo);

            fputs("<tr>",archivo);
            fputs("<td>Fecha Creacion",archivo);
            fputs("</td>",archivo);
            fputs("<td>",archivo);
            if(ebr.part_fit=='b')
            {
                fputs("BestFit",archivo);
            }
            else if(ebr.part_fit=='f')
            {
                fputs("FirstFit",archivo);
            }
            else
            {
                fputs("WorstFit",archivo);
            }
            fputs("</td>",archivo);
            fputs("</tr>",archivo);

            fputs("<tr>",archivo);
            fputs("<td>Inicio",archivo);
            fputs("</td>",archivo);
            fputs("<td>",archivo);
            sprintf(append,"%d",ebr.part_start); // put the int into a string
            strcpy(line,append);
            fputs(line,archivo);
            fputs("</td>",archivo);
            fputs("</tr>",archivo);

            fputs("<tr>",archivo);
            fputs("<td>Tamano",archivo);
            fputs("</td>",archivo);
            fputs("<td>",archivo);
            sprintf(append,"%d",ebr.part_size); // put the int into a string
            strcpy(line,append);
            fputs(line,archivo);
            fputs("</td>",archivo);
            fputs("</tr>",archivo);

            fputs("<tr>",archivo);
            fputs("<td>Siguiente",archivo);
            fputs("</td>",archivo);
            fputs("<td>",archivo);
            sprintf(append,"%d",ebr.part_next); // put the int into a string
            strcpy(line,append);
            fputs(line,archivo);
            fputs("</td>",archivo);
            fputs("</tr>",archivo);
            fputs("</table>",archivo);
        }
        fclose(temp);
    }
}

void graficarDISCO(MBR mbr,char *ruta)
{
    char line[1000];
    char append[50]; //New variable

    FILE *archivo=fopen("DISK.dot","w+");

    if(archivo)
    {
        fputs("digraph ArchivoMBR{\n",archivo);//inicio de archivo
        fputs("     subgraph cluster_DISCO{\n",archivo);
        for(int i =3; i>=0; i--)
        {
            fputs("         subgraph cluster_",archivo);
            fputc(letras[i],archivo);
            fputs("{\n",archivo);
            if(mbr.part[i]==True)
            {
                if(mbr.particion[i].part_type=='p')
                {
                    fputs("\n           ",archivo);
                    fputs("\"",archivo);
                    sprintf(append,"%d",contador_sub);
                    fputs(append,archivo);
                    fputs("\" ",archivo);
                    sprintf( append,"[label =  \"Primaria \n %s\",fontname = \"Verdana\",fillcolor=yellow,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 4];\n",mbr.particion[i].part_name);
                    fputs(append,archivo);
                }
                else if(mbr.particion[i].part_type=='e')
                {
                    fputs("\n           ",archivo);
                    fputs("\"",archivo);
                    sprintf(append,"%d",contador_sub);
                    fputs(append,archivo);
                    fputs("\" ",archivo);
                    sprintf( append,"[label = \"Extendida \n%s\",fontname = \"Verdana\",fillcolor=red,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 4];\n",mbr.particion[i].part_name);
                    fputs(append,archivo);
                    fputs("                 subgraph cluster_LOGICAS{",archivo);
                    EBR ebr;
                    int salir =False;
                    int inicio =mbr.particion[i].part_start;
                    FILE *temp =fopen(ruta,"rb+");
                    if(temp)
                    {
                        while(salir!=True)
                        {
                            EBR ebr;
                            contador_sub++;
                            fseek(temp,inicio,SEEK_CUR);
                            fread(&ebr,sizeof(EBR),1,temp);
                            rewind(temp);

                            if(ebr.part_next==-1)
                            {
                                fputs("\n           ",archivo);
                                fputs("\"",archivo);
                                sprintf(append,"%d",contador_sub);
                                fputs(append,archivo);
                                fputs("\" ",archivo);
                                sprintf(append,"[label = \"Logica \n%s\",fontname = \"Verdana\",fillcolor=green,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 1];\n",ebr.part_name);
                                fputs(append,archivo);
                                inicio =ebr.part_start+ebr.part_size;
                            }
                            else if(ebr.part_next==0)
                            {
                                salir=True;
                            }
                            else
                            {
                                fputs("\n           ",archivo);
                                fputs("\"",archivo);
                                sprintf(append,"%d",contador_sub);
                                fputs(append,archivo);
                                fputs("\" ",archivo);
                                sprintf(append,"[label = \"Logica \n %s\",fontname = \"Verdana\",fillcolor=green,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 1];\n",ebr.part_name);
                                fputs(append,archivo);
                                inicio =ebr.part_next;
                            }


                        }
                        fclose(temp);
                    }
                    fputs("\n               }\n",archivo);
                }

            }
            else
            {
                fputs("\n           ",archivo);
                fputs("\"",archivo);
                sprintf(append,"%d",contador_sub);
                fputs(append,archivo);
                fputs("\" ",archivo);
                fputs( "[label = \"Libre\",fontname = \"Verdana\",fillcolor=gray,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 4];\n",archivo);
            }
            fputs("\n        }\n",archivo);
            contador_sub++;
        }
        fputs("         subgraph cluster_MBR{",archivo);
        fputs("\n           ",archivo);
        fputs( "\"MBR\" [label =  \"MBR\",fontname = \"Verdana\",fillcolor=lightblue,style = filled,labelloc=c,shape = rectangle,fontsize = 15,height=5,width = 2];\n",archivo);
        fputs("\n       }",archivo);
        fputs("\n   }",archivo);
        fputs("\n}\n",archivo);

    }
    fclose(archivo);
    strcpy(line,"dot -Tpng DISK.dot -o ");
    strcat(line,pathArchivo);
    system(line);
}
#pragma endregion REP
