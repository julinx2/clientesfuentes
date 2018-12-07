/***********************************************************************/
/*                             CLIENTE                                  */
/*                      SERVIDOR:Cliente                                */
/*                                                                      */
/************************************************************************/
/* Omega Ingenieria de Software Ltda                                    */
/* Fecha de Desarrollo  : Febrero de 1998                               */
/************************************************************************/
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <oci.h>
#include <malloc.h>
#include <atmi.h>           /* TUXEDO Header File API's */
#include <userlog.h>        /* TUXEDO Header File */
#include <fml32.h>            /* TUXEDO FML Support */

#include <servidor.h>
#include <GestionDeErrores.h>
#include <olist.h>
#include <omegafml32.h>

#include <sgtFML.h>
#include <clienteFML.h>
#include <hermesFML.h>
#include <cuentaCorrienteFML.h>

#include <atalla_f2.h>
#include <tarjetaCredito.h>  
#include <tarjetaCredito_cons.h>

#include <cliente.h>
#include <clienteAutorizacion_fun.h>
#include <cliente_fun.h>
#include <cliente_sql.h>

/* INICIO LO NUEVO OCT-2011  */
#include <omsyslog.h>
/* FIN DE LO NUEVO OCT-2011 JHBC */

#define SIN_TIN "Sin TIN"
#define SIN_DIRECCION "Sin Direccion"
#define SIN_PAIS "000"

char gFechaProceso[8+1];
char gFechaCalendario[8+1];


tpsvrinit(int argc, char *argv[])
{
   long  sts;

   if (tpopen() == -1)
   {
      userlog("El Servidor de Cliente: \"Cliente\", fallo al conectarse con el Administrador de Base de Datos");
      exit(1);
   }

   userlog("El Servidor de Cliente: \"Cliente\", ha iniciado su ejecucion satisfactoriamente...");
   return 0;
}


void tpsvrdone()
{
   if (tpclose() == -1)
   {
      switch(tperrno)
      {
         case TPERMERR:
            userlog("El Servidor de Cliente: \"Cliente\", fallo al desconectarse del Administrador de Base de Datos para mas informacion consultar al manejador especifico");
            break;

         case TPEPROTO:
            userlog("El Servidor de Cliente: \"Cliente\", fallo al desconectarse del Administrador de Base de Datos debido a un problema de contexto del close()");
            break;

         case TPESYSTEM:
            userlog("El Servidor de Cliente: \"Cliente\", fallo al desconectarse del Administrador de Base de Datos debido a un problema con Tuxedo-System/T");
            break;

         case TPEOS:
            userlog("El Servidor de Cliente: \"Cliente\", fallo al desconectarse del Administrador de Base de Datos debido a un error del Sistema Operativo");
            break;

         default:
            userlog("El Servidor de Cliente: \"Cliente\", fallo al desconectarse del Administrador de Base de Datos debido a un error de excepcion en el sistema");
            break;
      }
	  return ;
   }
   userlog("El Servidor de Cliente: \"Cliente\", ha finalizado su ejecucion...");
   return ;
}

/************************************************************************/
/* Objetivo: Crear Cliente                                              */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica: Angélica Becerra             Fecha:  Abril   2002          */
/************************************************************************/
void CliInsCliente(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliCLIENTE cliente;
   tCliINFOADM informacion;
   typeSuperIntendencia super;
   int transaccionGlobal;
   char cadena[201];
   tCliREGMOD  regModificacion;
   tCliCLAVEDEACCESOIVR clave;
   /* [INI]: Veto General Rut */
   char clienteVetado[2+1];
   /* [FIN]: Veto General Rut */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;  
  
   LlenarRegistroCliente(fml, &cliente, &informacion, 1); 
   AsignarDeFMLASuper(fml, &super);
 
  /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   /* [INI]: Veto General Rut */

   sts = RecuperarClienteVetado(cliente.rut, clienteVetado);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion TuxRecuperarClienteVetado.", 0);
      userlog("%s: Error en funcion RecuperarClienteVetado.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(clienteVetado,"SI")==0)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no puede operar por política interna Banco Falabella.", 0);
      userlog("%s: Cliente no puede operar por política interna Banco Falabella.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name),(char *)fml, 0L, 0L);
   }

   /* [FIN]: Veto General Rut */

   sts = SqlRetornarClientePep(&cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "ERROR: No se puede retornar cliente pep.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliCrearCliente(cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear cliente - ERROR: No se pudo crear al cliente.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   
   sts = SqlCliCrearInfoAdm(informacion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear cliente - ERROR: No se pudo crear la informacion administrativa.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliCrearSuperIntendencia(super);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear cliente - ERROR: No se pudo crear la info. asociada a la superintendencia.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   
   /*grabar Reg. Modif.*/
   ConcatenaPersonal(cliente, cadena);
   regModificacion.tipoModificacion = CLI_CREACION_CLIENTE;
   LlenarRegistroRegMod(fml, &regModificacion, cadena);
   
   sts = SqlCliCrearRegMod(regModificacion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Insertar cliente - ERROR: No se logro insertar registro de modificación.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   clave.rutCliente = cliente.rut;
   clave.ejecutivo = regModificacion.usuario;
   clave.estado = CLI_CLAVE_INACTIVA;
   clave.estadoEmision = CLI_CLAVE_EMISION_EN_ESPERA;
   clave.pinOffset = 0;
   
   sts = SqlCliInsertarClaveDeAccesoIvr(clave);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente ya posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

 
/************************************************************************/
/* Objetivo: Modificar cliente                                          */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica: Angélica Becerra             Fecha: Abril   2002           */
/************************************************************************/
void CliModCliente(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   char cadena2[201];
   tCliCLIENTE cliente;
   tCliCLIENTE clienteAnterior;
   tCliINFOADM informacion;
   tCliREGMOD  regModificacion;
   tCliREGMOD  regModificacion2;
   typeSuperIntendencia  super;
   tcliClienteTributario   cliClienteTributario;

/* + EMS - INI/SYSLOG - ENE 2013 */
   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaCalendario[DIM_FECHA_HORA];
   time_t hora;
   long  identificadorUsuario;
   char  nombreUsuario[15 + 1];
   char flagAplicaSyslog[1 +1];
   char rutAux[10], dvAux[2], nombreAux[DIM_NOMBRE], apePaternoAux[DIM_APELLIDO], apeMaternoAux[DIM_APELLIDO];
   char correoAntAux[80+1], correoAux[80+1];

   memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));
   memset((char *)&cliClienteTributario, 0, sizeof(tcliClienteTributario));

/* - EMS - FIN/SYSLOG - ENE 2013 */
 
   memset((void *)&cliente,0,sizeof(tCliCLIENTE));
   memset((void *)&clienteAnterior,0,sizeof(tCliCLIENTE));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroCliente(fml, &cliente, &informacion, 2); 
   AsignarDeFMLASuper(fml, &super);

/* + EMS - INI/SYSLOG - ENE 2013 */

  if (Foccur32(fml,HRM_FLAG_APLICA_SYSLOG) > 0)
         Fget32(fml, HRM_FLAG_APLICA_SYSLOG, 0, flagAplicaSyslog, 0);
  else
      strcpy(flagAplicaSyslog,"N");

  if (Foccur32(fml,HRM_IP_ORIGEN) > 0)
      Fget32(fml, HRM_IP_ORIGEN, 0, bufferSysLog.ipOrigen, 0);

  if (Foccur32(fml,HRM_USUARIO_CONEXION) > 0)
      Fget32(fml, HRM_USUARIO_CONEXION, 0, bufferSysLog.usuarioDeAplicacion, 0);

  if (Foccur32(fml,HRM_USUARIO_RED) > 0)
      Fget32(fml, HRM_USUARIO_RED, 0, bufferSysLog.usuarioDeRed, 0);

/* - EMS - FIN/SYSLOG - ENE 2013 */

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   clienteAnterior.rut = cliente.rut; 
   sts = SqlCliRecuperarCliente(&clienteAnterior);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, 
          "Modificar cliente - ERROR: No se logro recuperar info. del cliente", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   /* Implantación de Normativa FATCA PJVA INI */
   if (strlen(cliente.codigoPais) == 0) strcpy(cliente.codigoPais, clienteAnterior.codigoPais);
   if (strlen(cliente.codigoPaisResidencia) == 0) strcpy(cliente.codigoPaisResidencia, clienteAnterior.codigoPaisResidencia);
   if (strlen(cliente.codigoPaisNacimiento) == 0) strcpy(cliente.codigoPaisNacimiento, clienteAnterior.codigoPaisNacimiento);
   /* Implantación de Normativa FATCA PJVA FIN */
   
   sts = SqlCliModificarCliente(cliente);
   if (sts != SQL_SUCCESS) 
   {
       if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente - ERROR: No se encontro el cliente", 0);
       else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente - ERROR: No se logro modificar el cliente", 0);
 
       if (transaccionGlobal == 0)
          tpabort(0L);
 
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (strcmp(cliente.clienteCrs,CLI_NO)==0){
      sts = SqlCliEliminarClientesCRS(cliente.rut);
      if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND) 
      {
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente - ERROR: No se logro modificar el cliente (clienteCRS)", 0);
 
          if (transaccionGlobal == 0)
             tpabort(0L);
 
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      if (sts == SQL_NOT_FOUND){

        cliClienteTributario.rut = cliente.rut;
        strcpy(cliClienteTributario.tin,SIN_TIN);
        strcpy(cliClienteTributario.domicilioCrs,SIN_DIRECCION);
        strcpy(cliClienteTributario.codPais,SIN_PAIS);

        sts=SqlCliAgregarClienteTributario(cliClienteTributario,CLI_NO);
        if (sts != SQL_SUCCESS){

            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al declarar que no posee direcciones tributarias", 0);
            if (transaccionGlobal == 0)
             tpabort(0L);
            tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
        }

      }

   }

   sts = SqlCliModificarSuperIntendencia(super);
   if (sts != SQL_SUCCESS)
   {
     if (sts == SQL_NOT_FOUND)
       Fadd32(fml, HRM_MENSAJE_SERVICIO, 
       "Modificar cliente - ERROR: No se encontro la info. asociada a la superintendencia.",0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, 
       "Modificar cliente - ERROR: No se logro modificar info. asociada a la superintendencia.",0);
 
     if (transaccionGlobal == 0)
          tpabort(0L);
 
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   
   /*grabar Reg. Modif.*/
   ConcatenaPersonal(clienteAnterior, cadena);
   regModificacion.tipoModificacion = CLI_MODIFICACION_PERSONALES;
   LlenarRegistroRegMod(fml, &regModificacion, cadena); 

   sts = SqlCliCrearRegMod(regModificacion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, 
        "Modificar cliente - ERROR: No se logro modificar la info. historica.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }


   /* ACD */
   /*grabar Reg. Modif.*/
   
   if (clienteAnterior.direccionElectronica == NULL && cliente.direccionElectronica != NULL)
		regModificacion2.tipoModificacion = CLI_INGRESO_CORREO_ELECTRONICO;
   else
		if (clienteAnterior.direccionElectronica == CLIENTE_NO_TIENE_EMAIL && cliente.direccionElectronica != NULL && cliente.direccionElectronica != CLIENTE_NO_TIENE_EMAIL)
			regModificacion2.tipoModificacion = CLI_ELIMINACION_CORREO_ELECTRONICO;
		else
			regModificacion2.tipoModificacion = CLI_MODIFICACION_CORREO_ELECTRONICO;
   
   
   ConcatenaCorreoElectronico(clienteAnterior, cadena2);
   LlenarRegistroRegMod(fml, &regModificacion2, cadena2); 

   sts = SqlCliCrearRegMod(regModificacion2);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, 
        "Modificar cliente - ERROR: No se logro modificar la info. historica.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   /* ACD */

/* + EMS - INI/SYSLOG - ENE 2013 */

    if (regModificacion2.tipoModificacion == CLI_MODIFICACION_CORREO_ELECTRONICO && (strcmp(flagAplicaSyslog,"S") == 0) &&
          strcmp(clienteAnterior.direccionElectronica, cliente.direccionElectronica)!= 0)
    {
         bufferSysLog.formatoLibre = CLI_FORMATO_CORREO;
         hora = (time_t)time((time_t *)NULL);
         strftime(fechaCalendario, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
         strcpy(bufferSysLog.fechaHora,fechaCalendario);
         bufferSysLog.codigoSistema = CLI_COD_SISTEMA_FICHA_CLIENTE;
         strcpy(bufferSysLog.nombreDeAplicacion,CLI_NOMBRE_SISTEMA_FICHA_CLIENTE);
         bufferSysLog.usuarioDeAplicacion[15] = 0;
         OmRecuperarIpServ(bufferSysLog.ipDestino);
         bufferSysLog.ipDestino[15] = 0;
         sprintf(bufferSysLog.mensajeDetalle , CLI_DESCRIPCION_ACCION_CAMBIO_CORREO);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         sprintf(rutAux, "%09li|%c", cliente.rut,OmCalcularDigitoVerificador(cliente.rut));
         strcat(bufferSysLog.mensajeDetalle ,rutAux);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         sprintf(nombreAux, "%-30s", cliente.nombres);
         sprintf(apePaternoAux, "%-20s", cliente.apellidoPaterno);
         sprintf(apeMaternoAux, "%-20s", cliente.apellidoMaterno);
         sprintf(correoAntAux, "%-80s", clienteAnterior.direccionElectronica);
         sprintf(correoAux, "%-80s", cliente.direccionElectronica);
         strcat(bufferSysLog.mensajeDetalle ,nombreAux);
         strcat(bufferSysLog.mensajeDetalle ,apePaternoAux);
         strcat(bufferSysLog.mensajeDetalle ,apeMaternoAux);  
         strcat(bufferSysLog.mensajeDetalle ,"| |");
         strcat(bufferSysLog.mensajeDetalle , correoAntAux);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         strcat(bufferSysLog.mensajeDetalle ,correoAux);
         strcat(bufferSysLog.mensajeDetalle ,"| | |");
         strcat(bufferSysLog.mensajeDetalle ,"|");

         /* Escribir en el SYSLOG.LOG */
          OmEscribirEnSysLog(&bufferSysLog);
      } /* FIN DEL IF*/
/* - EMS - INI/SYSLOG - ENE 2013 **********/


   if (transaccionGlobal == 0)
       tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}
 
/************************************************************************/
/* Objetivo: Obtener Cliente                                            */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica: Angélica Becerra             Fecha: Abril   2004           */
/************************************************************************/
void CliRecCliente(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   short existe=0;
   tCliCLIENTE cliente;
   tCliINFOADM informacion;
   tCliPARAMETRO parametro;
   typeSuperIntendencia super;
   int totalRegistros=0, i=0;
   char fechaProceso[20], fechaCal[20];
  /* CPM Ini Log */
   long canalReal=0;
   long  pCanalReal=0;
   long	 usuario=0;
   FBFR32   *fmlLog;
   long largo=0;
   char accion[150+1];
   long   rutCliente=0;
   /* CPM fin Log */
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   memset((char *)&cliente,0,sizeof(tCliCLIENTE));
   memset((char *)&informacion,0,sizeof(tCliINFOADM));
   memset((char *)&parametro,0,sizeof(tCliPARAMETRO));

   Fget32(fml, CLI_RUTX, 0, (char *)&cliente.rut, 0);
   /* CPM GET RUT START*/
   Fget32(fml, CLI_RUTX, 0, (char *)&rutCliente, 0);
   /* CPM GET RUT END */
   informacion.rutCliente = cliente.rut;
   super.rutCliente = cliente.rut;
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 /*CPM INI Log*/

   if (Foccur32(fml, HRM_CODIGO_SISTEMA) > 0) 
      Fget32(fml, HRM_CODIGO_SISTEMA, 0, (char *)&canalReal, 0);

   if (Foccur32(fml, HRM_IDENTIFICADOR_USUARIO) > 0) 
      Fget32(fml, HRM_IDENTIFICADOR_USUARIO, 0, (char *)&usuario, 0);

   if (Foccur32(fml, HRM_DESCRIPCION_ACCION) > 0)
      Fget32(fml, HRM_DESCRIPCION_ACCION, 0, accion, 0);
   
   Fprint32(fml);
   printf("\n-------- CPM rutCliente %li, canalReal %d, usuario  %d\n", rutCliente,canalReal,usuario );

   /* fmlLog = NewFml32((long)1, (short)6, (short)14, (short)7, (long)80); */
   fmlLog = (FBFR32 *)tpalloc("FML32",NULL, 1024);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al ejecutar la accion solicitada.", 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fmlLog, HRM_IDENTIFICADOR_USUARIO, (char *)&usuario, 0);
   /*pCanalReal = canalReal;*/
   Fadd32(fmlLog, HRM_CODIGO_SISTEMA, (char *)&canalReal, 0);
   Fadd32(fmlLog, HRM_RUT, (char *)&rutCliente, 0);
   Fadd32(fmlLog, HRM_DESCRIPCION_ACCION, accion, 0);

   printf("PASE CliRecCliente cliente.c2 ..1\n");
   Fprint32(fmlLog);
   printf("PASE CliRecCliente cliente.c2 ..2\n");
   printf("\n********** usuario , %d\n" ,usuario );
   if (usuario>0){
	   printf("tpcall HrmRegLog \n");
	   sts = tpcall("HrmRegLog", (char *)fmlLog, 0L, (char **)&fmlLog, &largo, TPNOTRAN);
	   if (sts == -1)
	   {
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en al invocar servicio HrmRegLog", 0);
			tpfree((char *)fmlLog);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
	   } 
	}   
   tpfree((char *)fmlLog);
   /*CPM FIN Log*/
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT, 0L) == -1)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar cliente - ERROR: Fallo al iniciar transaccion.", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }
 
     sts = SqlCliRecuperarCliente(&cliente);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente no existe.", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la información del cliente.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     sts = SqlCliTieneDeclaracionCRS(cliente.rut, cliente.clienteCrs);
     if (sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la informacion de clienteCRS.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     sts = SqlCliObtInfoAdm(&informacion);
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"No existe informacion administrativa del cliente.", 0);
       else
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se logro obtener la info. administrativa del cliente.",0);

       if (transaccionGlobal == 0)
          tpabort(0L);
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     sts = SqlCliRecuperarSuperIntendencia(&super);
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO,
            "Recuperar cliente - ERROR: No existe informacion asociada a la superintendencia",0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO,
        "Recuperar cliente - ERROR: No se logro obtener la info. asociada a la superintendencia",0);
 
       if (transaccionGlobal == 0)
          tpabort(0L);
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

      if (cliente.pep > 0){
        sts = SqlCliRecuperarMensajeClientePep(&cliente);
        if (sts == SQL_SUCCESS)
           cliente.pep = ES_PEP;
        else if (sts != SQL_NOT_FOUND){
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se logro obtener la info. de parametro.",0);

          if (transaccionGlobal == 0)
             tpabort(0L);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
        else
            cliente.pep = 0;
     }

     sts = SqlRecuperarParametrosSGT(fechaProceso, fechaCal);
     if (sts != SQL_SUCCESS)
     {
        userlog("Falla recuperacion de parametros de proceso (SGT)");
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }

     sts = SqlCliExisteClienteFatcaUS(cliente.rut, fechaProceso, &existe);
     if (sts != SQL_SUCCESS)
     {
        userlog("Falla al recuperar si es cliente fatca");
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla al determinar si el cliente es FATCA", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }

     if (existe > 0) cliente.esClienteFatcaUS = SI_NUMERICO;


     ObtieneRegistroCliente(fml, &cliente, &informacion); 
     AsignarAFMLDeSuper(fml, &super); 
 
     if (transaccionGlobal == 0)
         tpcommit(0L);

     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Crear Conyuge                                              */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsConyuge(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliCONYUGE conyuge;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;  
   
   LlenarRegistroConyuge(fml, &conyuge); 

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Conyuge.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearConyuge(conyuge);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El conyuge ya ha sido creado", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Conyuge", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Modificar conyuge                                          */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModConyuge(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliCONYUGE conyuge;
   tCliCONYUGE conyugeAnterior;
   tCliREGMOD  regModificacion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroConyuge(fml, &conyuge);

 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Conyuge.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   conyugeAnterior.rutCliente = conyuge.rutCliente;
   sts = SqlCliRecuperarConyuge(&conyugeAnterior);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Cónyuge.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarConyuge(conyuge);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Conyuge", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaConyuge(conyugeAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_CONYUGE;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }
}
 
 
/************************************************************************/
/* Objetivo: Obtener Conyuge                                           */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecConyuge(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCONYUGE conyuge;
   int totalRegistros=0, i=0;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&conyuge.rutCliente, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar Conyuge", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarConyuge(&conyuge);
 
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Conyuge", 0);
       else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Conyuge", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
 
      ObtieneRegistroConyuge(fml, &conyuge);
 
      if (transaccionGlobal == 0)
         tpcommit(0L);
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
 
/************************************************************************/
/* Objetivo: Crear Actividad Laboral                                    */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsActividad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliACTIVIDAD actividad;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&actividad,0,sizeof(tCliACTIVIDAD));
 
   LlenarRegistroActividad(fml, &actividad);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Activida Laboral.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearActividadLaboral(actividad);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La Actividad  Laboral ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Actividad Laboral", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
 
/************************************************************************/
/* Objetivo: Modificar Actividad                                        */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModActividad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliACTIVIDAD actividad;
   tCliACTIVIDAD actividadAnterior;
   char cadena[221];
   tCliREGMOD  regModificacion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroActividad(fml, &actividad);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, 
          "Error al iniciar Transaccion Modificar actividad.", 0);
         tpreturn(TPFAIL,ErrorServicio(SRV_CRITICAL_ERROR,rqst->name),(char *)fml,0L,0L);
      }

   actividadAnterior.rutCliente = actividad.rutCliente;
   sts = SqlCliRecuperarActividadLaboral(&actividadAnterior); 
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarActividadLaboral(actividad);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Actividad Laboral", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaActividad(actividadAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_LABORAL;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);
 
      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
   }
}
 
 
/************************************************************************/
/* Objetivo: Obtener Actividad Laboral                                  */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecActividad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliACTIVIDAD actividad;
   int totalRegistros=0, i=0;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&actividad.rutCliente, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar Actividad", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarActividadLaboral(&actividad);
 
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Actividad Laboral", 0);
       else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Actividad", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
 
      ObtieneRegistroActividad(fml, &actividad);
 
      if (transaccionGlobal == 0)
         tpcommit(0L);
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
 
/************************************************************************/
/* Objetivo: Crear una Direccion                                        */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDireccion(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliDIRECCION direccion;
   tCliREGMOD regModificacion;
   char cadena[201];
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&direccion,0,sizeof(tCliDIRECCION));
   memset((char *)&regModificacion,0,sizeof(tCliREGMOD));
 
   LlenarRegistroDireccion(fml, &direccion);

   if (strlen(direccion.departamento) == 0) strcpy(direccion.departamento, " ");
   if (strlen(direccion.codigoPostal) == 0) strcpy(direccion.codigoPostal, " ");
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
       Fadd32(fml,HRM_MENSAJE_SERVICIO, "Crear direccion - ERROR: Fallo al iniciar transaccion.", 0);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********* ContinuidadHoraria [FIN] *********/

   sts = SqlCliCrearDireccion(direccion);
 
   if (sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear direccion - ERROR: No se logro crear la direccion", 0);
 
     if (transaccionGlobal == 0) tpabort(0L);
 
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   strcpy(cadena, "Creación de dirección");

   if (strcmp(rqst->name, "CliInsEnvioEECC") == 0)
   {

      if (direccion.tipoDireccion == CLI_DIRECCION_PARTICULAR)
         regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_PARTICULAR;
      else
      {
         if (direccion.tipoDireccion == CLI_DIRECCION_COMERCIAL)
            regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_COMERCIAL;
         else
            regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_ALTERNATIVA;
      }
   }
   else
      regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION;

   LlenarRegistroRegMod(fml, &regModificacion, cadena);

   strcpy(regModificacion.fecha, gFechaProceso);

   sts = SqlCliCrearRegMod(regModificacion);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

      if (transaccionGlobal == 0) tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (transaccionGlobal == 0) tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
 
/************************************************************************/
/* Objetivo: Modificar direccion                                        */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModDireccion(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   char cadena[201];
   long sts;
   tCliDIRECCION direccion;
   tCliDIRECCION direccionAnterior;
   tCliREGMOD regModificacion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&direccion,0,sizeof(tCliDIRECCION));
   memset((char *)&direccionAnterior,0,sizeof(tCliDIRECCION));
   memset((char *)&regModificacion,0,sizeof(tCliREGMOD));
 
   LlenarRegistroDireccion(fml, &direccion);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, 
        "Error al iniciar Transaccion Modificar Direccion.", 0);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

   direccionAnterior.rutCliente = direccion.rutCliente;
   direccionAnterior.tipoDireccion = direccion.tipoDireccion;


   sts = SqlCliRecuperarUnaDireccion(&direccionAnterior);
   if (sts != SQL_SUCCESS)
   {

      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarDireccion(direccion);
 
   if (sts != SQL_SUCCESS)
   {
     if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Direccion", 0);
     else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
     if (transaccionGlobal == 0) tpabort(0L);
 
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaDireccion(direccionAnterior, cadena);

      if (strcmp(rqst->name, "CliModEnvioEECC") == 0)
      {
         if (direccion.tipoDireccion == CLI_DIRECCION_PARTICULAR)
            regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_PARTICULAR;
         else
         {
            if (direccion.tipoDireccion == CLI_DIRECCION_COMERCIAL)
               regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_COMERCIAL;
            else
               regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_ALTERNATIVA;
         }
      }
      else
         regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION;

      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0) tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
       if (transaccionGlobal == 0) tpcommit(0L);
 
       tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
   }
}
 
/************************************************************************/
/* Objetivo: Obtener   Direcciones                                      */
/* Autor   : Claudia Reyes                Fecha: Julio 1998             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDireccion(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long rut;
   tCliDIRECCION *direccion;
   char opcionRecuperacion[2];
   short tipoDireccion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }
 

   direccion = (tCliDIRECCION *)calloc(CLI_MAX_DIREC, 
                sizeof(tCliDIRECCION));
   if (direccion == NULL)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
     if (transaccionGlobal == 0)
       tpabort(0L);
     tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   if (!strncmp ( opcionRecuperacion , CLI_TODOS , 1 ))
     sts = SqlCliRecuperarDirecciones(&direccion, rut, 
                     CLI_MAX_DIREC, &totalRegistros);
   else
   {
     Fget32(fml, CLI_TIPO_DIRECCION, 0,(char *)&direccion->tipoDireccion, 0);
     totalRegistros = 1;
     direccion->rutCliente = rut;
     sts = SqlCliRecuperarUnaDireccion(direccion);
   }
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Direccion", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(direccion);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
 
    fml = (FBFR32 *)tpalloc("FML32", NULL, 
          ESTIMAR_MEMORIA(1 + 12 * totalRegistros, 
          sizeof(int) + sizeof(tCliDIRECCION) * totalRegistros));
 
    ObtenerRegistroDirecciones(fml, direccion, totalRegistros);
    free(direccion);

    if (transaccionGlobal == 0)
      tpcommit(0L);
 
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

 
/************************************************************************/
/* Objetivo: Eliminar Direccion                                         */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmDireccion(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliDIRECCION direccion;
   int totalRegistros=0, i, largoRetorno;
   char cadena[201];
   tCliREGMOD  regModificacion;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_RUTX, 0, (char *)&direccion.rutCliente, 0);
   Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccion.tipoDireccion, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, 
        "Error al iniciar Transaccion Eliminar Direccion.", 0);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

   sts = SqlCliRecuperarUnaDireccion(&direccion);
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Direccion", 0);
       else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recupera Direccion", 0);
 
       if (transaccionGlobal == 0)
          tpabort(0L);
 
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      sts = SqlCliEliminarDireccion(direccion);
 
      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
 
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaDireccion(direccion, cadena);
         regModificacion.tipoModificacion = CLI_ELIMINACION_DIRECCION;
         LlenarRegistroRegMod(fml, &regModificacion, cadena); 

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            if (sts == SQL_DUPLICATE_KEY)
               Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
            else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
            if (transaccionGlobal == 0)
               tpabort(0L);
 
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
           if (transaccionGlobal == 0)
              tpcommit(0L);
           tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }
}
 
 
/************************************************************************/
/* Objetivo: Crear un Telefono                                          */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsTelefono(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts,stsAnterior;
   tCliTELEFONO telefono,telAnterior;
   char cadena[201];
   tCliREGMOD  regModificacion;
   int transaccionGlobal;
   int contador=0,i;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&telefono,0,sizeof(tCliTELEFONO));
   memset((char *)&telAnterior,0,sizeof(tCliTELEFONO));
   memset((char *)&regModificacion,0,sizeof(tCliREGMOD));
 
   Fget32(fml, CLI_RUTX, 0, (char *)&telefono.rutCliente, 0);
   telAnterior.rutCliente = telefono.rutCliente;
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
            "Insertar telefonos - ERROR: Fallo al iniciar la transaccion.",0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   contador = Foccur32(fml,CLI_TIPO_TELEFONO);
   for (i=0; i<contador; i++)
   {
     LlenarRegistroTelefono(fml,&telefono,i);

     telAnterior.tipoTelefono = telefono.tipoTelefono;
     stsAnterior = SqlCliRecuperarUnTelefono(&telAnterior);
     if (stsAnterior == SQL_SUCCESS)
     {
        sts = SqlCliEliminarTelefono(telAnterior);
        if (sts != SQL_SUCCESS)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO,
                "Insertar telefonos - ERROR:  Fallo al eliminar la informacion existente.",0);

            if (transaccionGlobal == 0)
                 tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }

         /*grabar Reg. Modif.*/
         ConcatenaTelefono(telAnterior, cadena);
         regModificacion.tipoModificacion = CLI_MODIFICACION_TELEFONO;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, 
              "Insertar telefonos - ERROR: Fallo al registrar la informacion en el historico.",0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
     }

     sts = SqlCliCrearTelefono(telefono);
 
     if (sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, 
          "Insertar telefonos - ERROR: Fallo al insertar telefonos.",0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
 
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
   }

   if (transaccionGlobal == 0)
         tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

 
/************************************************************************/
/* Objetivo: Modificar Telefono                                         */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModTelefono(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTELEFONO telefono;
   tCliTELEFONO telefonoAnterior;
   char cadena[201], numeroTelefonoAux[20+1];
   tCliREGMOD  regModificacion;

/* + EMS - INI/SYSLOG - ENE 2013 */
   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaCalendario[DIM_FECHA_HORA];
   time_t hora;
   long  identificadorUsuario;
   char  nombreUsuario[15 + 1];
   char flagAplicaSyslog[1 +1];
   char rutAux[10], dvAux[2];

   memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));

/* - EMS - FIN/SYSLOG - ENE 2013 */
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroTelefono(fml, &telefono, 0);

/* + EMS - INI/SYSLOG - ENE 2013 */

  if (Foccur32(fml,HRM_FLAG_APLICA_SYSLOG) > 0)
         Fget32(fml, HRM_FLAG_APLICA_SYSLOG, 0, flagAplicaSyslog, 0);
  else
      strcpy(flagAplicaSyslog,"N");

  if (Foccur32(fml,HRM_IP_ORIGEN) > 0)
      Fget32(fml, HRM_IP_ORIGEN, 0, bufferSysLog.ipOrigen, 0);

  if (Foccur32(fml,HRM_USUARIO_CONEXION) > 0)
      Fget32(fml, HRM_USUARIO_CONEXION, 0, bufferSysLog.usuarioDeAplicacion, 0);

  if (Foccur32(fml,HRM_USUARIO_RED) > 0)
      Fget32(fml, HRM_USUARIO_RED, 0, bufferSysLog.usuarioDeRed, 0);

/* - EMS - FIN/SYSLOG - ENE 2013 */


   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Telefono.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   telefonoAnterior.rutCliente = telefono.rutCliente;
   telefonoAnterior.tipoTelefono = telefono.tipoTelefono;
   sts = SqlCliRecuperarUnTelefono(&telefonoAnterior);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
         sts = SqlCliCrearTelefono(telefono);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Insertar telefonos - ERROR: Fallo al insertar telefonos.",0);

            if (transaccionGlobal == 0) tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar el teléfono anterior", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }
   else
   {
      sts = SqlCliModificarTelefono(telefono);
      if (sts != SQL_SUCCESS)
      {
        if (sts == SQL_NOT_FOUND)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Telefono", 0);
        else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   /*grabar Reg. Modif.*/
   ConcatenaTelefono(telefonoAnterior, cadena);
   regModificacion.tipoModificacion = CLI_MODIFICACION_TELEFONO;
   LlenarRegistroRegMod(fml, &regModificacion, cadena); 

   sts = SqlCliCrearRegMod(regModificacion);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
/* + EMS - INI/SYSLOG - ENE 2013 */

      if (telefono.tipoTelefono == CLI_TIPO_TELEFONO_CELULAR && strcmp(flagAplicaSyslog,"S") == 0 &&
          strcmp(telefonoAnterior.numero, telefono.numero)!= 0)
      {
         bufferSysLog.formatoLibre = 1;
         hora = (time_t)time((time_t *)NULL);
         strftime(fechaCalendario, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
         strcpy(bufferSysLog.fechaHora,fechaCalendario);
         bufferSysLog.codigoSistema = CLI_COD_SISTEMA_FICHA_CLIENTE;
         strcpy(bufferSysLog.nombreDeAplicacion,CLI_NOMBRE_SISTEMA_FICHA_CLIENTE);
     
         if (bufferSysLog.usuarioDeAplicacion == '')
         {
           bufferSysLog.usuarioDeAplicacion[0] = 0;
           identificadorUsuario = (regModificacion.usuario == 0)? 1:regModificacion.usuario;
           sts = OmRecNombreUsuarioHrm(identificadorUsuario,nombreUsuario);
           if(sts != SRV_SUCCESS)
           {
            printf("Fallo al recuperar el nombre con funcion OmRecNombreUsuarioHrm identificador=[%d]\n",identificadorUsuario);
           }
           strncpy(bufferSysLog.usuarioDeAplicacion, nombreUsuario,15);
         }
         bufferSysLog.usuarioDeAplicacion[15] = 0;
         OmRecuperarIpServ(bufferSysLog.ipDestino);
         bufferSysLog.ipDestino[15] = 0;
         telefonoAnterior.nombreCliente[70] = 0;
         telefonoAnterior.numero[20]=0;
         telefono.numero[20]=0;
         sprintf(numeroTelefonoAux, "%-20s", telefono.numero);
         sprintf(bufferSysLog.mensajeDetalle , CLI_DESCRIPCION_ACCION_CAMBIO_CELULAR);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         sprintf(rutAux, "%09li|%c", regModificacion.rutCliente,OmCalcularDigitoVerificador(regModificacion.rutCliente));
         strcat(bufferSysLog.mensajeDetalle ,rutAux);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         strcat(bufferSysLog.mensajeDetalle ,telefonoAnterior.nombreCliente);
         strcat(bufferSysLog.mensajeDetalle ,"| | | |");
         strcat(bufferSysLog.mensajeDetalle , telefonoAnterior.numero);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         strcat(bufferSysLog.mensajeDetalle ,numeroTelefonoAux);
         strcat(bufferSysLog.mensajeDetalle ,"|");
         strcat(bufferSysLog.mensajeDetalle ,"|");
         /* Escribir en el SYSLOG.LOG */
          OmEscribirEnSysLog(&bufferSysLog);

      } /* FIN DEL IF*/
/* - EMS - INI/SYSLOG - ENE 2013 **********/

   if (transaccionGlobal == 0)
         tpcommit(0L);
 
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
 
/************************************************************************/
/* Objetivo: Eliminar Telefono                                          */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmTelefono(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTELEFONO telefono;
   char cadena[201];
   tCliREGMOD  regModificacion;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_RUTX, 0, (char *)&telefono.rutCliente, 0);
   Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefono.tipoTelefono, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar Telefono.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   sts = SqlCliRecuperarUnTelefono(&telefono);
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Telefono", 0);
       else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recupera Telefono", 0);
 
       if (transaccionGlobal == 0)
          tpabort(0L);
 
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      sts = SqlCliEliminarTelefono(telefono);
 
      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
 
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaTelefono(telefono, cadena);
         regModificacion.tipoModificacion = CLI_ELIMINACION_TELEFONO;
         LlenarRegistroRegMod(fml, &regModificacion, cadena); 

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            if (sts == SQL_DUPLICATE_KEY)
               Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
            else
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
            if (transaccionGlobal == 0)
               tpabort(0L);
 
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
            if (transaccionGlobal == 0)
               tpcommit(0L);
            tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }
}
 
/************************************************************************/
/* Objetivo: Obtener   Telefonos                                        */
/* Autor   : Claudia Reyes                Fecha: Julio 1998             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecTelefono(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long   rut;
   short  tipoFono;
   char opcionRecuperacion[2];
   tCliTELEFONO *telefono;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
 
  telefono = (tCliTELEFONO *)calloc(CLI_MAX_TEL, sizeof(tCliTELEFONO));
  if (telefono == NULL)
  {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
      if (transaccionGlobal == 0)
         tpabort(0L);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
  }
 
  if (!strncmp ( opcionRecuperacion , CLI_TODOS , 1 ))
    sts=SqlCliRecuperarTelefonos(&telefono,rut,CLI_MAX_TEL,&totalRegistros);
  else
  {
     Fget32(fml, CLI_TIPO_TELEFONO, 0,(char *)&telefono->tipoTelefono, 0);
     totalRegistros = 1;
     telefono->rutCliente = rut;

     sts = SqlCliRecuperarUnTelefono(telefono);
  }
 
  if (sts != SQL_SUCCESS)
  {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el telefono", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(telefono);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
 
   fml = (FBFR32 *)tpalloc("FML32", NULL, ESTIMAR_MEMORIA(1 + 12 * totalRegistros, 
         sizeof(int) + sizeof(tCliTELEFONO) * totalRegistros));
 
   ObtenerRegistroTelefonos(fml, telefono, totalRegistros);
   free(telefono);
 
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
 
/************************************************************************/
/* Objetivo : Obtener prefijos de celular                               */
/* Autor    : Ivan Espindola                Fecha: Noviembre 2011       */
/* Modifica :                               Fecha:                      */
/************************************************************************/
/* ACD */
void CliRecPrefijCel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   int totalRegistros=0;
   long sts;
   
   tCliPREFIJOSCELULAR *prefijo;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
        Fadd32(fml,HRM_MENSAJE_SERVICIO,
            "Error al iniciar Transaccion Recuperar Prefijos", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
    prefijo = (tCliPREFIJOSCELULAR *)calloc(CLI_MAX_PRE_CEL, sizeof(tCliPREFIJOSCELULAR));
	
	if (prefijo == NULL)
    {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar Prefijos Celular - Error inesperado en la aplicacion.", 0);
      if (transaccionGlobal == 0)
         tpabort(0L);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }
	
	sts = SqlCliRecuperarPrefijosCelular(&prefijo, CLI_MAX_PRE_CEL, &totalRegistros);

    if (sts != SQL_SUCCESS)
    {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encontraron registros", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
      
         if (transaccionGlobal == 0)
             tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }

    fml = (FBFR32 *)tpalloc("FML32", NULL, ESTIMAR_MEMORIA(1 + 12 * totalRegistros,
       sizeof(int) + sizeof(tCliPREFIJOSCELULAR) * totalRegistros));

 
    ObtenerRegistroPrefijos(fml, prefijo, totalRegistros);
    free(prefijo);

     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/* ACD */
 
 
/************************************************************************/
/* Objetivo: Obtener   Reg. Modificacion                                */
/* Autor   : Claudia Reyes                Fecha: Julio 1998             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRegistMod(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   long rut, correlativo;
   int totalRegistros=0, i, largoRetorno;
   tCliREGMOD *regMod;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }
 
   regMod = (tCliREGMOD *)calloc(CLI_MAX_REG_MOD,sizeof(tCliREGMOD));
   if (regMod == NULL)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
     if (transaccionGlobal == 0)
       tpabort(0L);
     tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   sts = SqlCliRecuperarRegModificacion(&regMod, rut, correlativo, CLI_MAX_REG_MOD, &totalRegistros);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No  existe Registros Modificados", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(regMod);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
 
   fml = (FBFR32 *)tpalloc("FML32", NULL, ESTIMAR_MEMORIA(1 + 12 * totalRegistros, sizeof(int) + sizeof(tCliREGMOD) * totalRegistros));
 
    ObtenerRegistroModificacion(fml, regMod, totalRegistros);
    free(regMod);
 
    if (transaccionGlobal == 0)
      tpcommit(0L);
 
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 




/******************Servicios InfoRequerida******************************/

/************************************************************************/
/* Objetivo: Crear InfoReq Cliente                                      */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDCLIENTE indCliente;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndCliente(fml, &indCliente);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Cliente.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearIndCliente(indCliente);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador cliente ya ha sido creado", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador cliente", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Modificar IndReq. Cliente                                  */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCLIENTE indCliente;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndCliente(fml, &indCliente);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Ind. Cliente.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarIndCliente(indCliente);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Cliente", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Obtener InfoReq.Cliente                                    */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCLIENTE indCliente;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indCliente.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar IndReq. Cliente", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarIndCliente(&indCliente);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Cliente", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Cliente", 0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     ObtieneRegistroIndCliente(fml, &indCliente);
 
     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar IndReq. Cliente                                   */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmInfReqCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCLIENTE indCliente;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_SISTEMA, 0, indCliente.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar IndReq. Cliente.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliEliminarIndCliente(indCliente);
 
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         if (transaccionGlobal == 0)
            tpcommit(0L);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
}

/************************************************************************/
/* Objetivo: Crear InfoReq Conyuge                                      */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqCyg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDCONYUGE indConyuge;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndConyuge(fml, &indConyuge);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Conyuge.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearIndConyuge(indConyuge);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador conyuge ya ha sido creado", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador conyuge", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Modificar InfoReq Conyuge                                  */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqCyg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCONYUGE indConyuge;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndConyuge(fml, &indConyuge);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Ind. Conyuge.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarIndConyuge(indConyuge);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Conyuge", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Obtener InfoReq.Conyuge                                    */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqCyg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCONYUGE indConyuge;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indConyuge.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion RecuperarIndReq. Conyuge", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarIndConyuge(&indConyuge);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Conyuge", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Conyuge", 0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     ObtieneRegistroIndConyuge(fml, &indConyuge);
 
     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
/************************************************************************/
/* Objetivo: Eliminar IndReq. Conyuge                                   */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmInfReqCyg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDCONYUGE indConyuge;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_SISTEMA, 0, indConyuge.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar IndReq. Conyuge.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliEliminarIndConyuge(indConyuge);
 
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         if (transaccionGlobal == 0)
            tpcommit(0L);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
}
 
/************************************************************************/
/* Objetivo: Crear InfoReq Direccion                                    */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqDir(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDDIRECCION indDireccion;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndDireccion(fml, &indDireccion);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind.Direccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearIndDireccion(indDireccion);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador Direccion ya ha sido creado ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador Direccion", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 

 
/************************************************************************/
/* Objetivo: Modificar InfoReq Direccion                                */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqDir(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDDIRECCION indDireccion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndDireccion(fml, &indDireccion);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Ind. Direccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarIndDireccion(indDireccion);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Direccion", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
 
/************************************************************************/
/* Objetivo: Obtener InfoReq. Direccion                                 */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqDir(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDDIRECCION indDireccion;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indDireccion.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar IndReq. Direccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarIndDireccion(&indDireccion);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Direccion", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Direccion", 0);
 
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     ObtenerRegistroIndDirecciones(fml, &indDireccion);
 
     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Eliminar IndReq. Direccion                                 */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmInfReqDir(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDDIRECCION indDireccion;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_SISTEMA, 0, indDireccion.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar IndReq. Direccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliEliminarIndDireccion(indDireccion);
 
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         if (transaccionGlobal == 0)
            tpcommit(0L);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
}

/************************************************************************/
/* Objetivo: Crea IndReq. Actividad Laboral                             */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqAct(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDACTIVIDAD indActividad;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndActividad(fml, &indActividad);
 
   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Actividad.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearIndActividad(indActividad);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador Actividad ya ha sido creado", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador Actividad", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Modificar InfoReq Act. Laboral                             */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqAct(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDACTIVIDAD indActividad;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndActividad(fml, &indActividad);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al iniciar Transaccion Modificar Ind. Actividad.",0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarIndActividad(indActividad);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Actividad", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 
/************************************************************************/
/* Objetivo: Obtener InfoReq.Actividad Laboral                          */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqAct(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDACTIVIDAD indActividad;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indActividad.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO,
              "Error al iniciar Transaccion Recuperar IndReq. Act. Laboral", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarIndActividad(&indActividad);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Actividad", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Actividad", 0) ;
 
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     ObtieneRegistroIndActividad(fml, &indActividad);
 
     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
/************************************************************************/
/* Objetivo: Eliminar IndReq. Actividad Laboral                         */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmInfReqAct(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDACTIVIDAD indActividad;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_SISTEMA, 0, indActividad.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar IndReq. Actividad.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliEliminarIndActividad(indActividad);
 
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         if (transaccionGlobal == 0)
            tpcommit(0L);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
}

/************************************************************************/
/* Objetivo: Crear InfoReq Telefono                                     */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqTel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDTELEFONO indTelefono;
   int transaccionGlobal;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndTelefono(fml, &indTelefono);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Telefono.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliCrearIndTelefono(indTelefono);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador Telefono ya ha sido creado ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador Telefono", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Modificar InfoReq Telefono                                 */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqTel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDTELEFONO indTelefono;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   LlenarRegistroIndTelefono(fml, &indTelefono);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion ModificarInd. Telefono.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarIndTelefono(indTelefono);
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Telefono", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);
 
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}
 

 
/************************************************************************/
/* Objetivo: Obtener InfoReq.Telefono                                   */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqTel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDTELEFONO indTelefono;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indTelefono.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar IndReq. Telefono", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliRecuperarIndTelefono(&indTelefono);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Telefono", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Telefono", 0) ;
 
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     ObtieneRegistroIndTelefono(fml, &indTelefono);
 

     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
/************************************************************************/
/* Objetivo: Eliminar IndReq. Telefono                                  */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmInfReqTel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDTELEFONO indTelefono;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_SISTEMA, 0, indTelefono.sistema, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar IndReq. Telefono.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
      sts = SqlCliEliminarIndTelefono(indTelefono);
 
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         if (transaccionGlobal == 0)
            tpcommit(0L);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
}
 


/*----  ANDRES   ------------------------------------*/
/************************************************************************/
void CliRecCodGlosa(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long rut;
   tCliCODIGOGLOSA *codGlosa;
   char nombreTabla[50];
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_NOMBRE_TABLA,0,nombreTabla,0);

 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   codGlosa = (tCliCODIGOGLOSA *)calloc(MAXIMA_ACTIVIDAD, 
                               sizeof(tCliCODIGOGLOSA));
   if (codGlosa == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
      if (transaccionGlobal == 0)
        tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = EjecutarObtGlosas(nombreTabla,&codGlosa, MAXIMA_ACTIVIDAD, 
                                 &totalRegistros);

    if (sts != SQL_SUCCESS)
    {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encontraron registros", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(codGlosa);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     fml = (FBFR32 *)tpalloc("FML32", NULL, 
         ESTIMAR_MEMORIA(1 + 12 * totalRegistros, sizeof(int) + 
         sizeof(tCliCODIGOGLOSA) * totalRegistros));
 

   /******   Buffer de salida   ******/
   for (i=0; i<totalRegistros; i++)
   {
     Fadd32(fml,CLI_FLAG_TIPO_EMPLEO,(char *)&(codGlosa + i)->otroCodigo,0);
     Fadd32(fml,CLI_TABLA_CODIGO,(char *)&(codGlosa + i)->codigo,0);
     Fadd32(fml,CLI_TABLA_DESCRIPCION,(codGlosa + i)->descripcion,0);
   } 
   free(codGlosa);
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L); 
}


void CliRecZonas(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   short comuna;
   tCliCODIGOGLOSA *codGlosa;
   char nombreTabla[50];
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_COMUNA,0,(char *)&comuna,0);

 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   codGlosa = (tCliCODIGOGLOSA *)calloc(MAXIMA_ACTIVIDAD, 
                               sizeof(tCliCODIGOGLOSA));
   if (codGlosa == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
      if (transaccionGlobal == 0)
        tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliObtZonas(comuna,&codGlosa, MAXIMA_ACTIVIDAD,&totalRegistros);

    if (sts != SQL_SUCCESS)
    {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encontraron registros", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(codGlosa);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
 
     fml = (FBFR32 *)tpalloc("FML32", NULL, 
         ESTIMAR_MEMORIA(1 + 12 * totalRegistros, sizeof(int) + 
         sizeof(tCliCODIGOGLOSA) * totalRegistros));
 

   /******   Buffer de salida   ******/
   for (i=0; i<totalRegistros; i++)
   {
     Fadd32(fml,CLI_TABLA_CODIGO,(char *)&(codGlosa + i)->codigo,0);
     Fadd32(fml,CLI_TABLA_DESCRIPCION,(codGlosa + i)->descripcion,0);
   } 
   free(codGlosa);
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L); 
}



/************************************************************************/
/* Objetivo: Obtener FechasParametro                                    */
/* Autor   :Andres Inzunza M.             Fecha: Septiembre 1998        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecParametro(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   short ultimaVersion;
   tCliPARAMETRO parametro;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
        Fadd32(fml,HRM_MENSAJE_SERVICIO,
            "Error al iniciar Transaccion Recuperar Parametro", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
    sts = SqlCliRecuperarParametro(&parametro);

    if (sts != SQL_SUCCESS)
    {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encontraron registros", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
      
         if (transaccionGlobal == 0)
             tpabort(0L);
 
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
    
     FMLSetearDesdeParametro(fml, &parametro);
 

     if (transaccionGlobal == 0)
         tpcommit(0L);
 
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 

/************************************************************************/
/* Objetivo: Inserta registros en la tabla BienesValores                */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsBienValor(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   tCliBIENVALOR bienValor;

   memset(&bienValor,0,sizeof(bienValor));
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_BIEN_TIPO);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 

   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, 
            "Error al iniciar Transaccion Crear Bien Valor.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
  
   Fget32(fml,CLI_RUTX,0,(char *)&bienValor.rut,0);

   sts = SqlCliEliminarBienYValorTodos(bienValor.rut);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
     Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error en servidor central. No se pudo eliminar la informacion actual de los bienes y valores",0);
     if (transaccionGlobal == 0)
        tpabort(0L);
   }

   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroBienValor(fml, &bienValor, i);
      sts = SqlCreaBienValor(&bienValor);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Bien valor ya ha sido creado", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear bienes valores", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modifica registros en la tabla BienesValores               */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModBienValor(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   char cadena[201];
   tCliBIENVALOR bienValor;
   tCliBIENVALOR bienValorAnterior;
   tCliREGMOD    regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_BIEN_TIPO);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Bien Valor.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&bienValor.rut,0);
   bienValorAnterior.rut = bienValor.rut;
   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroBienValor(fml, &bienValor, i);
      bienValorAnterior.tipoBien = bienValor.tipoBien;
      sts = SqlRecUnBienValor(&bienValorAnterior);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar bienes valores",0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      sts = SqlModiBienValor(&bienValor);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar bienes valores",0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }

      /*grabar Reg. Modif.*/
      ConcatenaBienValor(bienValorAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_BIEN_VALOR;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
          
  
/************************************************************************/
/* Objetivo: Recupera registros de la tabla BienesValores               */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecBienValor(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   long     sts;
   int      transaccionGlobal;
   long     rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar Bienes Valores.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecBienValor(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros bienes valores", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista); 
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearBienesValores(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista); 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/************************************************************************/
/* Objetivo: Inserta registros en la tabla BienRaiz                     */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsBienRaiz(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   tCliBIENRAIZ bienRaiz;

   memset(&bienRaiz,0,sizeof(bienRaiz));
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_CASA_UBICACION);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 

   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Bien Raiz.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
  
   Fget32(fml,CLI_RUTX,0,(char *)&bienRaiz.rutCliente,0);

   sts = SqlCliEliminarBienRaizTodos(bienRaiz.rutCliente);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
     Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error en servidor central. No se pudo eliminar la informacion actual de los bienes raices",0);
     if (transaccionGlobal == 0)
        tpabort(0L);
   }

   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroBienRaiz(fml, &bienRaiz, i, 0);
      sts = SqlLeerCorrelativoBienRaiz(&bienRaiz.correlativo);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al leer correlativo bien raiz",0);
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }

      sts = SqlCreaBienRaiz(&bienRaiz);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear bienes raices", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modifica registros en la tabla BienRaiz                    */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModBienRaiz(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   char cadena[201];
   tCliBIENRAIZ bienRaiz;
   tCliBIENRAIZ bienRaizAnterior;
   tCliREGMOD   regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_CASA_UBICACION);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Bien Raiz.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&bienRaiz.rutCliente,0);
   bienRaizAnterior.rutCliente = bienRaiz.rutCliente;
   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroBienRaiz(fml, &bienRaiz, i, 1);
      bienRaizAnterior.correlativo = bienRaiz.correlativo;
      sts = SqlRecUnBienRaiz(&bienRaizAnterior);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar bien raiz", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      sts = SqlModiBienRaiz(&bienRaiz);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml,HRM_MENSAJE_SERVICIO,"Rut y correlativo no encontrado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar bien Raiz", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }

      /*grabar Reg. Modif.*/
      ConcatenaBienRaiz(bienRaizAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_BIEN_RAIZ;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
          
  
/************************************************************************/
/* Objetivo: Recupera registros de la tabla BienRaiz                    */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecBienRaiz(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   long     sts;
   int      transaccionGlobal;
   long     rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar Bien Raiz.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecBienRaiz(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros bien raiz",0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista); 
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearBienRaiz(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista); 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Elimina registros en la tabla BienRaiz                     */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmBienRaiz(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   char cadena[201];
   tCliBIENRAIZ bienRaiz;
   tCliBIENRAIZ bienRaizAnterior;
   tCliREGMOD   regModificacion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_CORRELATIVO);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar Bien Raiz.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }

   Fget32(fml,CLI_RUTX,0,(char *)&bienRaiz.rutCliente,0);
   bienRaizAnterior.rutCliente = bienRaiz.rutCliente;
   for(i=0; i < totalRegistros; i++)
   {
      Fget32(fml,CLI_CORRELATIVO,i,(char *)&bienRaiz.correlativo,0);
      bienRaizAnterior.correlativo = bienRaiz.correlativo;
      sts = SqlRecUnBienRaiz(&bienRaizAnterior);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar bien raiz", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      sts = SqlCliEliminarBienRaiz(&bienRaiz);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml,HRM_MENSAJE_SERVICIO,"Rut o correlativo no encontrado", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar bien Raiz", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      /*grabar Reg. Modif.*/
      ConcatenaBienRaiz(bienRaizAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_BIEN_RAIZ;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
      }
   }
 
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta registros en la tabla Vehiculo                     */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsAuto(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   tCliAUTO vehiculo;

   memset(&vehiculo,0,sizeof(vehiculo));
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_AUTO_MARCA);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 

   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, 
            "Error al iniciar Transaccion Crear Vehiculo.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
  
   Fget32(fml,CLI_RUTX,0,(char *)&vehiculo.rutCliente,0);

   sts = SqlCliEliminarVehiculoTodos(vehiculo.rutCliente);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
     Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error en servidor central. No se pudo eliminar la informacion actual de los vehiculos",0);
     if (transaccionGlobal == 0)
        tpabort(0L);
   }

   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroAuto(fml, &vehiculo, i, 0);
      sts = SqlLeerCorrelativoAuto(&vehiculo.correlativo);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al leer correlativo vehiculo",0);
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }

      sts = SqlCreaAuto(&vehiculo);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear registro vehiculo", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modifica registros en la tabla Vehiculo                    */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModAuto(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   char cadena[201];
   tCliAUTO     vehiculo;
   tCliAUTO     vehiculoAnterior;
   tCliREGMOD   regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_AUTO_MARCA);
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Vehiculo.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&vehiculo.rutCliente,0);
   vehiculoAnterior.rutCliente = vehiculo.rutCliente;
   for(i=0; i < totalRegistros; i++)
   {
      LlenarRegistroAuto(fml, &vehiculo, i, 1);
      vehiculoAnterior.correlativo = vehiculo.correlativo;
      sts = SqlRecUnAuto(&vehiculoAnterior);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar vehiculo", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      sts = SqlModiAuto(&vehiculo);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml,HRM_MENSAJE_SERVICIO,"Rut y correlativo no encontrado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar vehiculo", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      /*grabar Reg. Modif.*/
      ConcatenaAuto(vehiculoAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_VEHICULO;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
      }
   } 

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
          
  
/************************************************************************/
/* Objetivo: Recupera registros de la tabla Vehiculo                    */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Recupera:                              Fecha:                        */
/************************************************************************/
void CliRecAuto(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   long     sts;
   int      transaccionGlobal;
   long     rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
  
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar Vehiculo.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }
   
   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecAuto(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registro vehiculo",0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista); 
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearAuto(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista); 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Elimina registros en la tabla Vehiculo                     */
/* Autor   : Ivan Oyaneder.               Fecha:  Octubre 1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmAuto(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int  i;
   int  totalRegistros;
   int  transaccionGlobal;
   char cadena[201];
   tCliAUTO     vehiculo;
   tCliAUTO     vehiculoAnterior;
   tCliREGMOD   regModificacion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   totalRegistros = Foccur32(fml, CLI_CORRELATIVO);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar Vehiculo.", 0);
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
   }

   Fget32(fml,CLI_RUTX,0,(char *)&vehiculo.rutCliente,0);
   vehiculoAnterior.rutCliente = vehiculo.rutCliente;
   for(i=0; i < totalRegistros; i++)
   {
      Fget32(fml,CLI_CORRELATIVO,i,(char *)&vehiculo.correlativo,0);
      vehiculoAnterior.correlativo = vehiculo.correlativo;
      sts = SqlRecUnAuto(&vehiculoAnterior);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar vehiculo", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      sts = SqlCliEliminarAuto(&vehiculo);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml,HRM_MENSAJE_SERVICIO,"Rut o correlativo no encontrado", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar vehiculo", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
 
         tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
      }
      /*grabar Reg. Modif.*/
      ConcatenaAuto(vehiculoAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_VEHICULO;
      LlenarRegistroRegMod(fml, &regModificacion, cadena); 

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
         if (transaccionGlobal == 0)
            tpabort(0L);
      }
   }
 
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Crear Vivienda                                             */
/* Autor   :                              Fecha:  Noviembre   1998      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsVivienda(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   typeVivienda vivienda;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;  
   
   AsignarDeFMLAVivienda(fml, &vivienda); 

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Vivienda.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearVivienda(vivienda);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error, la vivienda ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Vivienda", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}



void CliModVivienda(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   typeVivienda vivienda, viviendaActual;
   tCliREGMOD  regModificacion;
   char cadena[201];
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;  
   
   AsignarDeFMLAVivienda(fml, &vivienda); 

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, 
               "Error al iniciar Transaccion Modificar vivienda.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }


   viviendaActual.rutCliente = vivienda.rutCliente;
   sts = SqlCliRecuperarVivienda(&viviendaActual);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar vivienda", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }


   sts = SqlCliModificarVivienda(vivienda);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la vivienda", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar vivienda", 0);
 
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   ConcatenaVivienda(viviendaActual, cadena);
   regModificacion.tipoModificacion = CLI_MODIFICACION_VIVIENDA;
   LlenarRegistroRegMod(fml, &regModificacion, cadena);
 
   sts = SqlCliCrearRegMod(regModificacion);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);
 
      if (transaccionGlobal == 0)
            tpabort(0L);
 
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



void CliRecVivienda(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   typeVivienda vivienda;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&vivienda.rutCliente, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, 
            "Error al iniciar Transaccion Recuperar vivienda", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliRecuperarVivienda(&vivienda);
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error, no existe el vivienda", 0);
       else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar vivienda", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
 
      AsignarAFMLDeVivienda(fml, &vivienda);
 
      if (transaccionGlobal == 0)
         tpcommit(0L);
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/*---------------------ServiciosTablasCodigoDescripcion-----------------*/
/************************************************************************/
/* Objetivo: Agregar una ocurrencia de codigo descripcion               */
/* Autor   : JAS                                   Fecha: 25-11-1998    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliInsCodDes(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCODIGODESCRIPCION codigoDescripcion;
 
   /********   Buffer de entrada    ********/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&codigoDescripcion.codigo, 0);
   Fget32(fml, CLI_TABLA_TIPO_EMPLEO, 0, (char *)&codigoDescripcion.tipoEmpleo, 0);
   Fget32(fml, CLI_TABLA_DESCRIPCION, 0, codigoDescripcion.descripcion, 0);

   /****** Cuerpo del servicio ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name,"CliInsActiv") == 0)
      sts = SqlCliInsertarActividad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsBien") == 0)
      sts = SqlCliInsertarBien(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsCargo") == 0)
      sts = SqlCliInsertarCargo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsEstCiv") == 0)
      sts = SqlCliInsertarEstadoCivil(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsGirEmp") == 0)
      sts = SqlCliInsertarGiroEmpresa(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsNacion") == 0)
      sts = SqlCliInsertarNacionalidad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsNivEdu") == 0)
      sts = SqlCliInsertarNivelEducacional(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsProfesion") == 0)
      sts = SqlCliInsertarProfesion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsRegMat") == 0)
      sts = SqlCliInsertarRegimenMatrimonial(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsSexo") == 0)
      sts = SqlCliInsertarSexo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsTipDir") == 0)
      sts = SqlCliInsertarTipoDireccion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsTipTel") == 0)
      sts = SqlCliInsertarTipoTelefono(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsTipViv") == 0)
      sts = SqlCliInsertarTipoVivienda(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsZona") == 0)
      sts = SqlCliInsertarZona(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsHabito") == 0)
      sts = SqlCliInsertarHabitoPago(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliInsSituaCta") == 0)
      sts = SqlCliInsertarSituacionCuenta(&codigoDescripcion);
   else
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Servicio inexistente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El codigo ya existe", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
/************************************************************************/
/* Objetivo: Eliminar una ocurrencia de codigo descripcion              */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliElmCodDes(TPSVCINFO *rqst)
{
 
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCODIGODESCRIPCION codigoDescripcion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&codigoDescripcion.codigo, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name,"CliElmActiv") == 0)
      sts = SqlCliEliminarActividad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmBien") == 0)
      sts = SqlCliEliminarBien(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmCargo") == 0)
      sts = SqlCliEliminarCargo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmEstCiv") == 0)
      sts = SqlCliEliminarEstadoCivil(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmGirEmp") == 0)
      sts = SqlCliEliminarGiroEmpresa(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmNacion") == 0)
      sts = SqlCliEliminarNacionalidad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmNivEdu") == 0)
      sts = SqlCliEliminarNivelEducacional(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmProfesion") == 0)
      sts = SqlCliEliminarProfesion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmRegMat") == 0)
      sts = SqlCliEliminarRegimenMatrimonial(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmSexo") == 0)
      sts = SqlCliEliminarSexo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmTipDir") == 0)
      sts = SqlCliEliminarTipoDireccion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmTipTel") == 0)
      sts = SqlCliEliminarTipoTelefono(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmTipViv") == 0)
      sts = SqlCliEliminarTipoVivienda(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmZona") == 0)
      sts = SqlCliEliminarZona(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmHabito") == 0)
      sts = SqlCliEliminarHabitoPago(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliElmSituaCta") == 0)
      sts = SqlCliEliminarSituacionCuenta(&codigoDescripcion);
   else
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Servicio inexistente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
  
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar el registro deseado. Verifique que no se encuentre asociado a algún cliente.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 }
 
/************************************************************************/
/* Objetivo: Recuperar lista de ocurrencias codigo descripcion          */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliLisCodDes(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   Q_HANDLE *lista;
   int transaccionGlobal, i, totalRegistros;
   long sts;
   tCliCODIGODESCRIPCION *codigoDescripcion, temp;
 
   /****** Buffer de entrada *****/
   fml = (FBFR32 *)rqst->data;
 
   /****** Cuerpo del servicio ******/
   transaccionGlobal=tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   if (strcmp(rqst->name,"CliLisActiv") == 0)
      sts = SqlCliRecuperarActividadTodos(lista);
   else if (strcmp(rqst->name,"CliLisBien") == 0)
      sts = SqlCliRecuperarBienTodos(lista);
   else if (strcmp(rqst->name,"CliLisCargo") == 0)
      sts = SqlCliRecuperarCargoTodos(lista);
   else if (strcmp(rqst->name,"CliLisEstCiv") == 0)
      sts = SqlCliRecuperarEstadoCivilTodos(lista);
   else if (strcmp(rqst->name,"CliLisGirEmp") == 0)
      sts = SqlCliRecuperarGiroEmpresaTodos(lista);
   else if (strcmp(rqst->name,"CliLisNacion") == 0)
      sts = SqlCliRecuperarNacionalidadTodos(lista);
   else if (strcmp(rqst->name,"CliLisNivEdu") == 0)
      sts = SqlCliRecuperarNivelEducacionalTodos(lista);
   else if (strcmp(rqst->name,"CliLisProfesion") == 0)
      sts = SqlCliRecuperarProfesionTodos(lista);
   else if (strcmp(rqst->name,"CliLisRegMat") == 0)
      sts = SqlCliRecuperarRegimenMatrimonialTodos(lista);
   else if (strcmp(rqst->name,"CliLisSexo") == 0)
      sts = SqlCliRecuperarSexoTodos(lista);
   else if (strcmp(rqst->name,"CliLisTipDir") == 0)
      sts = SqlCliRecuperarTipoDireccionTodos(lista);
   else if (strcmp(rqst->name,"CliLisTipTel") == 0)
      sts = SqlCliRecuperarTipoTelefonoTodos(lista);
   else if (strcmp(rqst->name,"CliLisTipViv") == 0)
      sts = SqlCliRecuperarTipoViviendaTodos(lista);
   else if (strcmp(rqst->name,"CliLisZona") == 0)
      sts = SqlCliRecuperarZonaTodos(lista);
   else if (strcmp(rqst->name,"CliLisTipMtoRta") == 0)
      sts = SqlCliRecuperarTipoMontoRentaTodos(lista);
   else if (strcmp(rqst->name,"CliLisTipMtoEva") == 0)
      sts = SqlCliRecuperarTipoMontoRentaEva(lista);
   else if (strcmp(rqst->name,"CliLisHabito") == 0)
      sts = SqlCliRecuperarHabitoPagoTodos(lista);
   else if (strcmp(rqst->name,"CliLisSituaCta") == 0)
      sts = SqlCliRecuperarSituacionCuentaTodos(lista);
   else if (strcmp(rqst->name,"CliLisPariente") == 0)
      sts = SqlCliRecuperarParienteTodos(lista);
   else
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Servicio inexistente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
  
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
         if (strcmp(rqst->name,"CliRecTipViv") == 0)
         {
            codigoDescripcion = &temp;
            codigoDescripcion->codigo = 0;
            strcpy(codigoDescripcion->descripcion, "Otro");
         }
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Codigo no existe", 0); 
      }
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
 
   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliCODIGODESCRIPCION)* totalRegistros));
   for (i = 0; i < totalRegistros ; i++)
   {
      codigoDescripcion = QGetItem(lista, i);
      Fadd32(fml,CLI_TABLA_CODIGO, (char *)&codigoDescripcion->codigo,0);
      Fadd32(fml,CLI_TABLA_DESCRIPCION, codigoDescripcion->descripcion,0);
      if (strcmp(rqst->name,"CliLisActiv") == 0)
         Fadd32(fml,CLI_TIPO_EMPLEO, (char *)&codigoDescripcion->tipoEmpleo,0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
 
/************************************************************************/
/* Objetivo: Recuperar una ocurrencia de codigo descripcion             */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecCodDes(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCODIGODESCRIPCION codigoDescripcion;
 
   /***************  Buffer de Entrada  **********************/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&codigoDescripcion.codigo, 0);
 
   /***************  Cuerpo del Servicio *********************/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name,"CliRecActiv") == 0)
      sts = SqlCliRecuperarActividad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecBien") == 0)
      sts = SqlCliRecuperarBien(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecCargo") == 0)
      sts = SqlCliRecuperarCargo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecEstCiv") == 0)
      sts = SqlCliRecuperarEstadoCivil(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecGirEmp") == 0)
      sts = SqlCliRecuperarGiroEmpresa(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecNacion") == 0)
      sts = SqlCliRecuperarNacionalidad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecNivEdu") == 0)
      sts = SqlCliRecuperarNivelEducacional(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecProfesion") == 0)
      sts = SqlCliRecuperarProfesion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecRegMat") == 0)
      sts = SqlCliRecuperarRegimenMatrimonial(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecSexo") == 0)
      sts = SqlCliRecuperarSexo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecTipDir") == 0)
      sts = SqlCliRecuperarTipoDireccion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecTipTel") == 0)
      sts = SqlCliRecuperarTipoTelefono(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecTipViv") == 0)
      sts = SqlCliRecuperarTipoVivienda(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecZona") == 0)
      sts = SqlCliRecuperarZona(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecHabito") == 0)
      sts = SqlCliRecuperarHabitoPago(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliRecSituaCta") == 0)
      sts = SqlCliRecuperarSituacionCuenta(&codigoDescripcion);
   else
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Servicio inexistente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
         if (strcmp(rqst->name,"CliRecTipViv") == 0)
         {
            codigoDescripcion.codigo = 0;
            strcpy(codigoDescripcion.descripcion, "Otro"); 
         } 
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Codigo no existe", 0);
      }
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL,1024); 
   Fadd32(fml, CLI_TABLA_CODIGO, (char *)&codigoDescripcion.codigo, 0);
   Fadd32(fml, CLI_TABLA_TIPO_EMPLEO, (char *)&codigoDescripcion.tipoEmpleo, 0);
   Fadd32(fml, CLI_TABLA_DESCRIPCION, codigoDescripcion.descripcion, 0);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/************************************************************************/
/* Objetivo: Modificar ocurrencia de codigo descripcion                 */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliModCodDes(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCODIGODESCRIPCION codigoDescripcion;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&codigoDescripcion.codigo, 0);
   Fget32(fml, CLI_TABLA_TIPO_EMPLEO, 0, (char *)&codigoDescripcion.tipoEmpleo, 0);
   Fget32(fml, CLI_TABLA_DESCRIPCION, 0, codigoDescripcion.descripcion, 0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name,"CliModActiv") == 0)
      sts = SqlCliModificarActividad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModBien") == 0)
      sts = SqlCliModificarBien(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModCargo") == 0)
      sts = SqlCliModificarCargo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModEstCiv") == 0)
      sts = SqlCliModificarEstadoCivil(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModGirEmp") == 0)
      sts = SqlCliModificarGiroEmpresa(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModNacion") == 0)
      sts = SqlCliModificarNacionalidad(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModNivEdu") == 0)
      sts = SqlCliModificarNivelEducacional(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModProfesion") == 0)
      sts = SqlCliModificarProfesion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModRegMat") == 0)
      sts = SqlCliModificarRegimenMatrimonial(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModSexo") == 0)
      sts = SqlCliModificarSexo(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModTipDir") == 0)
      sts = SqlCliModificarTipoDireccion(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModTipTel") == 0)
      sts = SqlCliModificarTipoTelefono(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModTipViv") == 0)
      sts = SqlCliModificarTipoVivienda(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModZona") == 0)
      sts = SqlCliModificarZona(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModHabito") == 0)
      sts = SqlCliModificarHabitoPago(&codigoDescripcion);
   else if (strcmp(rqst->name,"CliModSituaCta") == 0)
      sts = SqlCliModificarSituacionCta(&codigoDescripcion);
   else
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Servicio inexistente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar todas las relaciones                             */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecRelsNePrf(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   Q_HANDLE *lista;
   int transaccionGlobal, i, totalRegistros;
   long sts;
   tCliRelacionNivelEducacionalProfesion *relacion;

   /****** Buffer de entrada *****/
   fml = (FBFR32 *)rqst->data;

   /****** Cuerpo del servicio ******/
   transaccionGlobal=tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }


   sts = SqlCliRecuperarRelacionesNivelEducacionalProfesion(lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - No existen relaciones entre Nivel Educacional y Profesión",0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error al recuperar relaciones", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliRelacionNivelEducacionalProfesion)* totalRegistros));
   for (i = 0; i < totalRegistros ; i++)
   {
      relacion = QGetItem(lista, i);
      Fadd32(fml,CLI_NIVEL_EDUCACIONAL, (char *)&relacion->nivelEducacional,0);
      Fadd32(fml,CLI_PROFESION, (char *)&relacion->profesion,0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar profesiones asociadas a un nivel educacional     */
/* Autor   : JAS                                      Fecha: 25-11-1998 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecPsRelUnNe(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   Q_HANDLE *lista;
   int transaccionGlobal, i, totalRegistros;
   short nivelEducacional=0;
   long sts;
   tCliCODIGODESCRIPCION *profesion;

   /****** Buffer de entrada *****/
   fml = (FBFR32 *)rqst->data;

   /****** Cuerpo del servicio ******/
   transaccionGlobal=tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave al recuperar profesiones relacionadas a este nivel educacional", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&nivelEducacional, 0);

   sts = SqlCliRecuperarProfesionesRelacionadasAUnNivelEducacional(nivelEducacional,lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - No existen profesiones relacionadas con este nivel educacional.",0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error al recuperar profesiones relacionadas a este nivel educacional", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliCODIGODESCRIPCION)* totalRegistros));
   for (i = 0; i < totalRegistros ; i++)
   {
      profesion = QGetItem(lista, i);
      Fadd32(fml,CLI_TABLA_CODIGO, (char *)&profesion->codigo,0);
      Fadd32(fml,CLI_TABLA_DESCRIPCION, profesion->descripcion,0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/**************************************************************************/
/* Objetivo: Mantención de relación nivel educ. / profesión.              */
/* Autor   :                                          Fecha: 19-05-1999   */
/* Modifica:                                          Fecha:              */
/**************************************************************************/
void CliModRelsNePrf(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliRelacionNivelEducacionalProfesion relacion;
   int totalRegistros=0, i=0;

   /****** Buffer de entrada *****/
   fml = (FBFR32 *)rqst->data;

   /****** Cuerpo del servicio ******/
   transaccionGlobal=tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   Fget32(fml, CLI_NIVEL_EDUCACIONAL, 0, (char *)&relacion.nivelEducacional, 0);

   sts = SqlEliminarRelacionNivelEducacionalProfesionTodas(relacion.nivelEducacional);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente - Se produjo un error grave al eliminar las relaciones. Los cambios realizados no han sido grabados", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   totalRegistros = Foccur32(fml, CLI_PROFESION);
   for (i=0; i < totalRegistros ; i++)
   {
      Fget32(fml, CLI_PROFESION, i, (char *)&relacion.profesion, 0);
      sts = SqlInsertarRelacionNivelEducacionalProfesion (&relacion);
      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml,HRM_MENSAJE_SERVICIO,"Cliente - Se produjo un error grave al actualizar las relaciones. Los cambios realizados no han sido grabados.",0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear InfoReq Telefono                                     */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqViv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDVIVIENDA indVivienda;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroIndVivienda(fml, &indVivienda);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Vivienda.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearIndVivienda(indVivienda);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El indicador Vivienda ya ha sido creado ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador Telefono", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Modificar InfoReq Telefono                                 */
/* Autor   : Claudia Reyes.               Fecha:  Julio   1998          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqViv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDVIVIENDA indVivienda;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroIndVivienda(fml, &indVivienda);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion ModificarInd. Vivienda.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarIndVivienda(indVivienda);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Ind. Vivienda", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}



/************************************************************************/
/* Objetivo: Obtener InfoReq.Telefono                                   */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqViv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDVIVIENDA indVivienda;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indVivienda.sistema, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar IndReq. Telefono", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

      sts = SqlCliRecuperarIndVivienda(&indVivienda);

     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Vivienda", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Vivienda", 0) ;

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     ObtieneRegistroIndVivienda(fml, &indVivienda);


     if (transaccionGlobal == 0)
         tpcommit(0L);

     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta renta del cliente                                  */
/* Autor   : A.B.G.M.                     Fecha: 30-08-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsRenta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts,stsAnterior;
   tCliRENTA renta,rentaAnterior;
   char cadena[201];
   tCliREGMOD  regModificacion;
   int transaccionGlobal;
   int contador=0,i;
   /* lo nuevo NOV-2013 SYSLOG JHBC */

   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaSyslog[DIM_FECHA_HORA];
   time_t hora;
   char   nombreAplicacion[30+1];
   char   nombreUsuario[15 + 1];
   double porcentajeM = 0.0;
   double montoRentaAnterior, montoRentaActual;
   char   producto[10 + 1];
   char   sPorcentajeM[15+1];
		 
   memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));

/* fin de lo nuevo NOV-2013 SYSLOG JHBC */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&renta.rutCliente, 0);
   rentaAnterior.rutCliente = renta.rutCliente;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
            "Insertar rentas - ERROR: Fallo al iniciar la transaccion.",0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   contador = Foccur32(fml,CLI_RTA_TIPO);
   for (i=0; i<contador; i++)
   {
     LlenarRegistroRenta(fml,&renta,i);

     rentaAnterior.tipoMontoRenta = renta.tipoMontoRenta;
     stsAnterior = SqlCliRecuperarUnaRenta(&rentaAnterior);
     if (stsAnterior == SQL_SUCCESS)
     {
        sts = SqlCliEliminarRenta(rentaAnterior);
        if (sts != SQL_SUCCESS)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO,
                "Insertar rentas - ERROR:  Fallo al eliminar la informacion existente.",0);

            if (transaccionGlobal == 0)
                 tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }

         /*grabar Reg. Modif.*/
         ConcatenaRenta(rentaAnterior, cadena);
         regModificacion.tipoModificacion = CLI_MODIFICACION_RENTA;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO,
              "Insertar rentas - ERROR: Fallo al registrar la informacion en el historico.",0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
		 /* lo nuevo NOV-2013 SYSLOG JHBC */

         montoRentaAnterior =  rentaAnterior.monto;
         montoRentaActual = renta.monto;


         if ( montoRentaAnterior != montoRentaActual && renta.tipoMontoRenta!= TIPO_MONTO_RENTA_ESTIMADA)
         { 
            bufferSysLog.formatoLibre = NNN_FORMATO_GENERAL;
            hora = (time_t)time((time_t *)NULL);
            strftime(fechaSyslog, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
            strcpy(bufferSysLog.fechaHora,fechaSyslog);
            bufferSysLog.codigoSistema = ID_SISTEMA_CLIENTE;
      
            sts = OmRecNombreSistemaHrm(bufferSysLog.codigoSistema, nombreAplicacion);
            if(sts != SRV_SUCCESS)
            {
               printf("Fallo al recuperar el nombreAplicacion con funcion OmRecNombreSistemaHrm codigoSistema=[%d]\n",bufferSysLog.codigoSistema);
            }
            printf("(CliInsRenta) nombreAplicacion =[%s]\n",nombreAplicacion);
            sts = OmRecNombreUsuarioHrm(regModificacion.usuario, nombreUsuario);
            if(sts != SRV_SUCCESS)
            {
               printf("Fallo al recuperar el nombreUsuario con funcion OmRecNombreUsuarioHrm movimiento.usuarioMovimiento=[%d]\n",regModificacion.usuario);
            }
            printf("(CliInsRenta) nombreUsuario =[%s]\n",nombreUsuario);


            strcpy(bufferSysLog.nombreDeAplicacion,nombreAplicacion);
            strncpy(bufferSysLog.usuarioDeAplicacion, nombreUsuario,15);
            bufferSysLog.usuarioDeAplicacion[15] = 0;
            strcpy(bufferSysLog.ipOrigen,IP_SIN_INFORMACION);
            bufferSysLog.ipOrigen[15] = 0;
            OmRecuperarIpServ(bufferSysLog.ipDestino);
            bufferSysLog.ipDestino[15] = 0;
            strncpy(bufferSysLog.usuarioDeRed,nombreUsuario,15);
            bufferSysLog.usuarioDeRed[15] = 0;

            sts = OmPorcentajeModificacion(montoRentaAnterior, montoRentaActual, &porcentajeM);
            if(sts != SRV_SUCCESS)
            {
               printf("Fallo al recuperar  al obtener Porcentaje de montoRentaAnterior=[%15.4f]/ montoRentaActual[%15.4f]\n",montoRentaAnterior, montoRentaActual);
            }
            printf("(CliInsRenta) porcentajeM=[%15.2f]\n",porcentajeM);

            switch(renta.tipoMontoRenta)
            {
               case  TIPO_MONTO_RENTA_DECLARADA:
                     strcpy(producto,PRODUCTO_RENTA_D);
                  break;
               case  TIPO_MONTO_RENTA_COMPROBADA:
                     strcpy(producto,PRODUCTO_RENTA_C);
                  break;
               default:
               break;
            }
		 
		    if (porcentajeM >= 0.0 )
		       sprintf(sPorcentajeM,"+%-14.2f",porcentajeM);
		    else
		       sprintf(sPorcentajeM,"%-15.2f",porcentajeM);
		 
            printf("(CliInsRenta) porcentajeM=[%15.4f]\n",porcentajeM);
            sprintf(bufferSysLog.mensajeDetalle,"%-16s|%15.2f|%15.2f|%15s|",producto,montoRentaAnterior,montoRentaActual,sPorcentajeM);
            sprintf(bufferSysLog.sRut,"%8li-%c",regModificacion.rutCliente,OmCalcularDigitoVerificador(regModificacion.rutCliente));
            strcat(bufferSysLog.mensajeDetalle,bufferSysLog.sRut);
            strcat(bufferSysLog.mensajeDetalle,"|");
            OmEscribirEnSysLog(&bufferSysLog);
         }

/* fin lo nuevo NOV-2013 SYSLOG JHBC */
     }

     if (renta.monto > 0 )
     {
        sts = SqlCliCrearRenta(renta);

        if (sts != SQL_SUCCESS)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO,
             "Insertar rentas - ERROR: Fallo al insertar rentas.",0);

           if (transaccionGlobal == 0)
              tpabort(0L);

           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
     }
   }

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}


/************************************************************************/
/* Objetivo: Modifica renta del cliente                                 */
/* Autor   : A.B.G.M.                     Fecha: 30-08-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModRenta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliRENTA renta, rentaAnterior;
   char cadena[201];
   tCliREGMOD  regModificacion;

/* lo nuevo NOV-2013 SYSLOG JHBC */

   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaSyslog[DIM_FECHA_HORA];
   time_t hora;
   char   nombreAplicacion[30+1];
   char   nombreUsuario[15 + 1];
   double porcentajeM = 0.0;
   double montoRentaAnterior, montoRentaActual;
   char   producto[10 + 1];
   char   sPorcentajeM[15+1];
		 
   memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));

/* fin de lo nuevo NOV-2013 SYSLOG JHBC */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&renta.rutCliente, 0);
   LlenarRegistroRenta(fml, &renta, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Renta.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }


   rentaAnterior.rutCliente = renta.rutCliente;
   rentaAnterior.tipoMontoRenta = renta.tipoMontoRenta;
   sts = SqlCliRecuperarUnaRenta(&rentaAnterior);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar renta - Error al generar historico.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarRenta(renta);
   if (sts != SQL_SUCCESS)
   {
     if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar renta - No existe tal tipo de renta", 0);
     else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {

      /*grabar Reg. Modif.*/
      ConcatenaRenta(rentaAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_RENTA;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);
      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar renta - Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {


/* lo nuevo NOV-2013 SYSLOG JHBC */

      montoRentaAnterior =  rentaAnterior.monto;
      montoRentaActual = renta.monto;


      if ( montoRentaAnterior != montoRentaActual && renta.tipoMontoRenta!= TIPO_MONTO_RENTA_ESTIMADA)
      { 
         bufferSysLog.formatoLibre = NNN_FORMATO_GENERAL;
         hora = (time_t)time((time_t *)NULL);
         strftime(fechaSyslog, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
         strcpy(bufferSysLog.fechaHora,fechaSyslog);
         bufferSysLog.codigoSistema = ID_SISTEMA_CLIENTE;
      
         sts = OmRecNombreSistemaHrm(bufferSysLog.codigoSistema, nombreAplicacion);
         if(sts != SRV_SUCCESS)
         {
            printf("Fallo al recuperar el nombreAplicacion con funcion OmRecNombreSistemaHrm codigoSistema=[%d]\n",bufferSysLog.codigoSistema);
         }
         printf("(CliModRenta) nombreAplicacion =[%s]\n",nombreAplicacion);
         sts = OmRecNombreUsuarioHrm(regModificacion.usuario, nombreUsuario);
         if(sts != SRV_SUCCESS)
         {
            printf("Fallo al recuperar el nombreUsuario con funcion OmRecNombreUsuarioHrm movimiento.usuarioMovimiento=[%d]\n",regModificacion.usuario);
         }
         printf("(CliModRenta) nombreUsuario =[%s]\n",nombreUsuario);


         strcpy(bufferSysLog.nombreDeAplicacion,nombreAplicacion);
         strncpy(bufferSysLog.usuarioDeAplicacion, nombreUsuario,15);
         bufferSysLog.usuarioDeAplicacion[15] = 0;
         strcpy(bufferSysLog.ipOrigen,IP_SIN_INFORMACION);
         bufferSysLog.ipOrigen[15] = 0;
         OmRecuperarIpServ(bufferSysLog.ipDestino);
         bufferSysLog.ipDestino[15] = 0;
         strncpy(bufferSysLog.usuarioDeRed,nombreUsuario,15);
         bufferSysLog.usuarioDeRed[15] = 0;

         sts = OmPorcentajeModificacion(montoRentaAnterior, montoRentaActual, &porcentajeM);
         if(sts != SRV_SUCCESS)
         {
            printf("Fallo al recuperar  al obtener Porcentaje de montoRentaAnterior=[%15.4f]/ montoRentaActual[%15.4f]\n",montoRentaAnterior, montoRentaActual);
         }
         printf("(CliModRenta) porcentajeM=[%15.2f]\n",porcentajeM);

         switch(renta.tipoMontoRenta)
         {
            case  TIPO_MONTO_RENTA_DECLARADA:
                  strcpy(producto,PRODUCTO_RENTA_D);
               break;
            case  TIPO_MONTO_RENTA_COMPROBADA:
                  strcpy(producto,PRODUCTO_RENTA_C);
               break;
            default:
            break;

         }
		 
		 if (porcentajeM >= 0.0 )
		    sprintf(sPorcentajeM,"+%-14.2f",porcentajeM);
		 else
		    sprintf(sPorcentajeM,"%-15.2f",porcentajeM);
		 
         printf("(CliModRenta) porcentajeM=[%15.4f]\n",porcentajeM);
         sprintf(bufferSysLog.mensajeDetalle,"%-16s|%15.2f|%15.2f|%15s|",producto,montoRentaAnterior,montoRentaActual,sPorcentajeM);
         sprintf(bufferSysLog.sRut,"%8li-%c",regModificacion.rutCliente,OmCalcularDigitoVerificador(regModificacion.rutCliente));
         strcat(bufferSysLog.mensajeDetalle,bufferSysLog.sRut);
         strcat(bufferSysLog.mensajeDetalle,"|");
         OmEscribirEnSysLog(&bufferSysLog);
      }

/* fin lo nuevo NOV-2013 SYSLOG JHBC */

      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
   }
}


/************************************************************************/
/* Objetivo: Elimina renta del cliente                                  */
/* Autor   : A.B.G.M.                     Fecha: 30-08-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmRenta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliRENTA renta;
   char cadena[201];
   tCliREGMOD  regModificacion;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&renta.rutCliente, 0);
   Fget32(fml, CLI_RTA_TIPO, 0, (char *)&renta.tipoMontoRenta, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar Renta.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   sts = SqlCliRecuperarUnaRenta(&renta);
   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar renta - No existe tal tipo de renta", 0);
       else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar renta - Error al recupera tipo de renta", 0);

       if (transaccionGlobal == 0)
          tpabort(0L);

       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      sts = SqlCliEliminarRenta(renta);

      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar renta - Error al Eliminar", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);

        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaRenta(renta, cadena);
         regModificacion.tipoModificacion = CLI_ELIMINACION_RENTA;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar renta - Error al crear Reg. Mod.", 0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
            if (transaccionGlobal == 0)
               tpcommit(0L);
            tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }
}

/************************************************************************/
/* Objetivo: Recupera renta del cliente                                 */
/* Autor   : A.B.G.M.                     Fecha: 30-08-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRentas(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long   rut;
   short  tipoMontoRenta;
   char opcionRecuperacion[2];
   tCliRENTA *renta;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar rentas - Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }


  renta = (tCliRENTA *)calloc(CLI_MAX_RENTA, sizeof(tCliRENTA));
  if (renta == NULL)
  {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar rentas - Error inesperado en la Aplicacion", 0);
      if (transaccionGlobal == 0)
         tpabort(0L);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
  }

  if (!strncmp ( opcionRecuperacion , CLI_TODOS , 1 ))
    sts=SqlCliRecuperarRentas(&renta,rut,CLI_MAX_RENTA,&totalRegistros);
  else
  {
     Fget32(fml, CLI_RTA_TIPO, 0,(char *)&renta->tipoMontoRenta, 0);
     totalRegistros = 1;
     renta->rutCliente = rut;

     sts = SqlCliRecuperarUnaRenta(renta);
  }

  if (sts != SQL_SUCCESS)
  {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar rentas - No existe el tipo de renta", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar rentas - Error al recuperar registros", 0);

      free(renta);
      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *)tpalloc("FML32", NULL, ESTIMAR_MEMORIA(1 + 12 * totalRegistros,
                         sizeof(int) + sizeof(tCliRENTA) * totalRegistros));

   ObtenerRegistroRentas(fml, renta, totalRegistros);
   free(renta);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear InfoReq Renta                                        */
/* Autor   : A.B.G.M.                     Fecha:  30-08-99              */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsInfReqRta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliINDRENTA indRenta;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroIndRenta(fml, &indRenta);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Ind. Renta.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearIndRenta(indRenta);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear indicador de renta", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Modificar InfoReq Renta                                    */
/* Autor   : A.B.G.M.                     Fecha:  30-08-99              */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModInfReqRta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDRENTA indRenta;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroIndRenta(fml, &indRenta);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion ModificarInd. Renta.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarIndRenta(indRenta);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar ind renta", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Recuperar InfoReq Renta                                    */
/* Autor   : A.B.G.M.                     Fecha:  30-08-99              */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecInfReqRta(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliINDRENTA indRenta;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_SISTEMA, 0, indRenta.sistema, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar IndReq. Renta", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliRecuperarIndRenta(&indRenta);

   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe los IndReq. Renta", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar IndReq. Renta", 0) ;

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   ObtieneRegistroIndRenta(fml, &indRenta);


   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecLstRelEmp(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lstRelacionEmpresa;
   tCliRELACIONEMPRESA *relacionEmpresa;

   lstRelacionEmpresa = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);
  
   sts =  SqlCliRecuperarRelacionEmpresaTodos(lstRelacionEmpresa);

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en relacion empresa", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar relacion empresa", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstRelacionEmpresa->count +1), sizeof(tCliRELACIONEMPRESA) * (lstRelacionEmpresa->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstRelacionEmpresa->count; i++)
   {
    relacionEmpresa = (tCliRELACIONEMPRESA *)QGetItem(lstRelacionEmpresa,i);

    Fadd32(fml, CLI_TABLA_CODIGO , (char *)&relacionEmpresa->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, relacionEmpresa->descripcion, 0);
   }

   QDelete(lstRelacionEmpresa);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecLstSIT32(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lstRelacionBanco;
   tCliSIRELACIONBANCOT32 *relacionBanco;

   lstRelacionBanco = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts =  SqlCliRecuperarRelacionBancoT32Todos(lstRelacionBanco);

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en relacion banco", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar relacion banco", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstRelacionBanco->count +1), sizeof(tCliSIRELACIONBANCOT32) * (lstRelacionBanco->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstRelacionBanco->count; i++)
   {
    relacionBanco = (tCliSIRELACIONBANCOT32 *)QGetItem(lstRelacionBanco,i);

    Fadd32(fml, CLI_TABLA_CODIGO , (char *)&relacionBanco->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, relacionBanco->descripcion, 0);
   }

   QDelete(lstRelacionBanco);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecLstSIT10(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lstActividadEconomica;
   tCliSIACTIVIDADECONOMICAT10 *actividadEconomica;

   lstActividadEconomica = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts =  SqlCliRecuperarActividadEconomicaT10Todos(lstActividadEconomica);

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en actividad economica", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar actividad economica", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstActividadEconomica->count +1), sizeof(tCliSIACTIVIDADECONOMICAT10) * (lstActividadEconomica->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstActividadEconomica->count; i++)
   {
    actividadEconomica = (tCliSIACTIVIDADECONOMICAT10 *)QGetItem(lstActividadEconomica,i);

    Fadd32(fml, CLI_TABLA_CODIGO , (char *)&actividadEconomica->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, actividadEconomica->descripcion, 0);
   }

   QDelete(lstActividadEconomica);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecLstSIT13(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lstClasificacionDeudor;
   tCliSICLASIFICACIONDEUDORT13 *clasificacionDeudor;

   lstClasificacionDeudor = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts =  SqlCliRecuperarClasificacionDeudorT13Todos(lstClasificacionDeudor);

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en clasificacion deudor", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar clasificacion deudor", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstClasificacionDeudor->count +1), sizeof(tCliSICLASIFICACIONDEUDORT13) * (lstClasificacionDeudor->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstClasificacionDeudor->count; i++)
   {
    clasificacionDeudor = (tCliSICLASIFICACIONDEUDORT13 *)QGetItem(lstClasificacionDeudor,i);

    Fadd32(fml, CLI_TABLA_CODIGO_STRING , clasificacionDeudor->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, clasificacionDeudor->descripcion, 0);
   }

   QDelete(lstClasificacionDeudor);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Funcionalidad : Servicio que recupera en una lista la composicion institucional */
/* Modificacion : Consuelo Montenegro (Omega Ing. Software)  Fecha: 20-06-2001     */
/***********************************************************************************/
void CliRecLstSIT11(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i, rutCliente=0;
   short        tipoEstrato;
   Q_HANDLE     *lstComposicionInstitucional;
   tCliSICOMPOSICIONINSTITUCIONALT11 *composicionInstitucional;

   lstComposicionInstitucional = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   if (Foccur32(fml, CLI_RUT) > 0)
   {
      Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);

      DeterminarComposicionInstitucional(rutCliente, &tipoEstrato);
      sts =  SqlCliRecuperarComposicionInstitucionalT11PorRut(tipoEstrato, lstComposicionInstitucional);
   }
   else
   {
       sts =  SqlCliRecuperarComposicionInstitucionalT11Todos(lstComposicionInstitucional);
   }

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en clasificacion deudor", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar clasificacion deudor", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstComposicionInstitucional->count +1), sizeof(tCliSICOMPOSICIONINSTITUCIONALT11) * (lstComposicionInstitucional->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstComposicionInstitucional->count; i++)
   {
    composicionInstitucional = (tCliSICOMPOSICIONINSTITUCIONALT11 *)QGetItem(lstComposicionInstitucional,i);

    Fadd32(fml, CLI_TABLA_CODIGO , (char *)&composicionInstitucional->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, composicionInstitucional->descripcion, 0);
   }

   QDelete(lstComposicionInstitucional);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecLstSIT06(TPSVCINFO *rqst)
{
   FBFR32         *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lstCategoriaDeudor;
   tCliCODIGOGLOSA  *categoriaDeudor;

   lstCategoriaDeudor = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts =  SqlCliRecuperarCategoriaDeudorT06Todos(lstCategoriaDeudor);

   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen datos en clasificacion deudor", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar clasificacion deudor", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA( 27 * (lstCategoriaDeudor->count +1), sizeof(tCliCODIGOGLOSA) * (lstCategoriaDeudor->count + 1)));

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstCategoriaDeudor->count; i++)
   {
    categoriaDeudor = (tCliCODIGOGLOSA *)QGetItem(lstCategoriaDeudor,i);

    Fadd32(fml, CLI_TABLA_CODIGO , (char *)&categoriaDeudor->codigo, 0);
    Fadd32(fml, CLI_TABLA_DESCRIPCION, categoriaDeudor->descripcion, 0);
   }

   QDelete(lstCategoriaDeudor);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Cliente Empresa                                      */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsEmpresa(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long               sts;
   tCliCLIENTEEMPRESA cliente;
   int                nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&cliente.rut, 0);
   Fget32(fml, CLI_DIGVER, 0, cliente.dv, 0);
   Fget32(fml, CLI_RAZON_SOCIAL, 0, cliente.razonSocial, 0);
   Fget32(fml, CLI_SI_CATEGORIA_DEUDOR, 0, (char *)&cliente.categoria, 0);
   Fget32(fml, CLI_COMPOSICION_INSTITUCIONAL, 0, (char *)&cliente.composicionInstitucional, 0);
   Fget32(fml, CLI_CLASIFICACION, 0, cliente.clasificacion, 0);
   Fget32(fml, CLI_FECHA_CLASIFICACION, 0, cliente.fechaClasificacion, 0);
   Fget32(fml, CLI_ACTIVIDAD_ECONOMICA, 0, (char *)&cliente.actividadEconomica, 0);
   Fget32(fml, CLI_RELACION_BANCO, 0, (char *)&cliente.relacionBanco, 0);
   Fget32(fml, CLI_RELACION_EMPRESA, 0, (char *)&cliente.relacionEmpresa, 0);
   Fget32(fml, CLI_EMAIL, 0, cliente.email, 0);
   sts = Fget32(fml, CLI_GIRO, 0, (char *)&cliente.giro, 0);
   if (sts == -1)
        cliente.giro = -1;

   sts = Fget32(fml, CLI_TIPO_SOCIEDAD_EMPRESA, 0, (char *)&cliente.sociedad, 0);
   if (sts == -1)
        cliente.sociedad = -1;

   cliente.estado = -1;

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearClienteEmpresa(&cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo ingresar el cliente empresa.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Cliente Empresa                                  */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModEmpresa(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long               sts;
   tCliCLIENTEEMPRESA cliente;
   int                nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUT, 0, (char *)&cliente.rut, 0);
   Fget32(fml, CLI_DIGVER, 0, cliente.dv, 0);
   Fget32(fml, CLI_RAZON_SOCIAL, 0, cliente.razonSocial, 0);
   Fget32(fml, CLI_SI_CATEGORIA_DEUDOR, 0, (char *)&cliente.categoria, 0);
   Fget32(fml, CLI_COMPOSICION_INSTITUCIONAL, 0, (char *)&cliente.composicionInstitucional, 0);
   Fget32(fml, CLI_CLASIFICACION, 0, cliente.clasificacion, 0);
   Fget32(fml, CLI_FECHA_CLASIFICACION, 0, cliente.fechaClasificacion, 0);
   Fget32(fml, CLI_ACTIVIDAD_ECONOMICA, 0, (char *)&cliente.actividadEconomica, 0);
   Fget32(fml, CLI_RELACION_BANCO, 0, (char *)&cliente.relacionBanco, 0);
   Fget32(fml, CLI_RELACION_EMPRESA, 0, (char *)&cliente.relacionEmpresa, 0);
   Fget32(fml, CLI_EMAIL, 0, cliente.email, 0);
   sts = Fget32(fml, CLI_GIRO, 0, (char *)&cliente.giro, 0);
   if (sts == -1)
        cliente.giro = -1;
   sts = Fget32(fml, CLI_TIPO_SOCIEDAD_EMPRESA, 0, (char *)&cliente.sociedad, 0);
   if (sts == -1)
        cliente.sociedad = -1;

   cliente.estado = -1;

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarClienteEmpresa(&cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo modificar el cliente.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Cliente Empresa                                   */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliEliEmpresa(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long               sts;
   tCliCLIENTEEMPRESA cliente;
   int                nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&cliente.rut, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliEliminarClienteEmpresa(&cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo eliminar el cliente.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar Cliente Empresa                                  */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecEmpresa(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long               sts;
   tCliCLIENTEEMPRESA cliente;
   int                nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&cliente.rut, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Cliente empresa - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliRecuperarClienteEmpresa(&cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo obtener cliente empresa.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUT, (char *)&cliente.rut, 0);
   Fadd32(fml, CLI_DIGVER, cliente.dv, 0);
   Fadd32(fml, CLI_RAZON_SOCIAL, cliente.razonSocial, 0);
   Fadd32(fml, CLI_SI_CATEGORIA_DEUDOR, (char *)&cliente.categoria, 0);
   Fadd32(fml, CLI_COMPOSICION_INSTITUCIONAL, (char *)&cliente.composicionInstitucional, 0);
   Fadd32(fml, CLI_CLASIFICACION, cliente.clasificacion, 0);
   Fadd32(fml, CLI_FECHA_CLASIFICACION, cliente.fechaClasificacion, 0);
   Fadd32(fml, CLI_ACTIVIDAD_ECONOMICA, (char *)&cliente.actividadEconomica, 0);
   Fadd32(fml, CLI_RELACION_BANCO, (char *)&cliente.relacionBanco, 0);
   Fadd32(fml, CLI_RELACION_EMPRESA, (char *)&cliente.relacionEmpresa, 0);
   Fadd32(fml, CLI_EMAIL, cliente.email, 0);

   Fadd32(fml, CLI_DESC_SI_CATEGORIA_DEUDOR, cliente.descripcionCategoria, 0);
   Fadd32(fml, CLI_DESC_COMPOSICION_INSTITUCIONAL, cliente.descripcionComposicionInstitucional, 0);
   Fadd32(fml, CLI_DESC_CLASIFICACION, cliente.descripcionClasificacion, 0);
   Fadd32(fml, CLI_DESC_ACTIVIDAD_ECONOMICA, cliente.descripcionActividadEconomica, 0);
   Fadd32(fml, CLI_DESC_RELACION_BANCO, cliente.descripcionRelacionBanco, 0);
   Fadd32(fml, CLI_DESC_RELACION_EMPRESA, cliente.descripcionRelacionEmpresa, 0);
   Fadd32(fml, CLI_ESTADO, (char *)&cliente.estado, 0);
   Fadd32(fml, CLI_FECHAVIGENCIA, cliente.fechaVigencia, 0);
   Fadd32(fml, CLI_FECHAVISACION, cliente.fechaVisacion, 0);
   Fadd32(fml, CLI_GIRO, (char *)&cliente.giro, 0);
   Fadd32(fml, CLI_TIPO_SOCIEDAD_EMPRESA, (char *)&cliente.sociedad, 0);

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Empresa Asociada                                     */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsEmpAsoci(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   rut;
   char   razonSocial[81];
   long   correlativo;
   int    nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
   Fget32(fml, CLI_RAZON_SOCIAL, 0, razonSocial, 0);
   Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearEmpresaAsociada(rut, razonSocial, correlativo);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo crear al empresa asociada.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Empresa Asociada                                 */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModEmpAsoci(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   rut;
   char   razonSocial[81];
   long   correlativo;
   int    nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
   Fget32(fml, CLI_RAZON_SOCIAL, 0, razonSocial, 0);
   Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarEmpresaAsociada(rut, razonSocial, correlativo);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo Modificar empresa asociada.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Empresa Asociada                                  */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliEliEmpAsoci(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   rut;
   long   correlativo;
   int    nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
   Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliEliminarEmpresaAsociada(rut, correlativo);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo eliminar empresa asociada.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar Empresa Asociada                                  */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecEmpAsoci(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   rut;
   long   correlativo;
   char   razonSocial[81];
   int    nivel;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
   Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliRecuperarEmpresaAsociada(rut, correlativo, razonSocial);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo recuperar empresa asociada.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RAZON_SOCIAL, razonSocial, 0);
   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar Empresa Asociada                                  */
/* Autor   : Tomas Alburquenque         Fecha:  Septiembre  1999        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecLstEmpAsc(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   tCliEMPRESARELACIONADA *empresaRelacionada;
   Q_HANDLE  *lstEmpresasRelacionadas;
   short i;
   long  rut;
   int    nivel;

   lstEmpresasRelacionadas = (Q_HANDLE *) QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);

   /******   Cuerpo del servicio   ******/

   nivel = tpgetlev();
   if (nivel == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO,
              "Crear cliente - ERROR: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliRecuperarEmpresaAsociadaTODAS(rut, lstEmpresasRelacionadas);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,
          "Crear cliente - ERROR: No se pudo recuperar empresa asociada.", 0);

      if (nivel == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 4096);

   if (fml == NULL)  /* Falla al estimar memoria */
   {
    fml = (FBFR32 *) tpalloc("FML32",NULL,ESTIMAR_MEMORIA(27,500));
    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);

    if (nivel == 0)
       tpabort(0L);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<lstEmpresasRelacionadas->count; i++)
   {
    empresaRelacionada = (tCliEMPRESARELACIONADA *)QGetItem(lstEmpresasRelacionadas,i);

    Fadd32(fml, CLI_RAZON_SOCIAL , empresaRelacionada->razonSocial, 0);
    Fadd32(fml, CLI_CORRELATIVO, (char *)&empresaRelacionada->correlativo, 0);
   }

   QDelete(lstEmpresasRelacionadas);

   if (nivel == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*****************************************************************************/
/******  MANEJO DE DIRECCIONES EMPRESA      **********************************/
/************************************************************************/
/* Objetivo: Crear una Direccion                                        */
/* Autor   : A.B.G.M.                     Fecha: 21-09-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDirEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliDIRECCION direccion;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroDireccion(fml, &direccion);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
       Fadd32(fml,HRM_MENSAJE_SERVICIO,"Crear direccion empresa - Error: Fallo al iniciar transaccion.", 0);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

   sts = SqlCliCrearDireccionEmpresa(direccion);

   if (sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo crear la dirección de la empresa.", 0);

     if (transaccionGlobal == 0)
        tpabort(0L);

     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
     if (transaccionGlobal == 0)
       tpcommit(0L);

     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}


/************************************************************************/
/* Objetivo: Modificar direccion                                        */
/* Autor   : A.B.G.M.                     Fecha: 21-09-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModDirEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   char cadena[201];
   long sts;
   tCliDIRECCION direccion;
   tCliDIRECCION direccionAnterior;
   tCliREGMOD regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroDireccion(fml, &direccion);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar direccion empresa - Error: Fallo al conectarse.", 0);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

   direccionAnterior.rutCliente = direccion.rutCliente;
   direccionAnterior.tipoDireccion = direccion.tipoDireccion;
   sts = SqlCliRecuperarUnaDireccionEmpresa(&direccionAnterior);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar direccion empresa - Error: Fallo al recuperar direccion.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   sts = SqlCliModificarDireccionEmpresa(direccion);
   if (sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar direccion empresa - Error: Fallo al modificar.", 0);

     if (transaccionGlobal == 0)
        tpabort(0L);

     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaDireccion(direccionAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_EMPRESA;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar direccion empresa - Error: Fallo al generar historico.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
       if (transaccionGlobal == 0)
       tpcommit(0L);

       tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
   }
}

/************************************************************************/
/* Objetivo: Obtener   Direcciones                                      */
/* Autor   : Claudia Reyes                Fecha: Julio 1998             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDirEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long rut;
   tCliDIRECCION *direccion;
   char opcionRecuperacion[2];
   short tipoDireccion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar direccion empresa - Error: Fallo al conectarse.", 0);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }


   direccion = (tCliDIRECCION *)calloc(CLI_MAX_DIREC, sizeof(tCliDIRECCION));
   if (direccion == NULL)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar direccion empresa - Error inesperado.", 0);
     if (transaccionGlobal == 0)
       tpabort(0L);
     tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   if (!strncmp ( opcionRecuperacion , CLI_TODOS , 1 ))
     sts = SqlCliRecuperarDireccionesEmpresa(&direccion, rut, CLI_MAX_DIREC, &totalRegistros);
   else
   {
     Fget32(fml, CLI_TIPO_DIRECCION, 0,(char *)&direccion->tipoDireccion, 0);
     totalRegistros = 1;
     direccion->rutCliente = rut;
     sts = SqlCliRecuperarUnaDireccionEmpresa(direccion);
   }

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar direccion empresa - Error: Fallo al recuperar direcciones.", 0);

      free(direccion);
      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }

    fml = (FBFR32 *)tpalloc("FML32", NULL,
          ESTIMAR_MEMORIA(1 + 12 * totalRegistros,
          sizeof(int) + sizeof(tCliDIRECCION) * totalRegistros));

    ObtenerRegistroDirecciones(fml, direccion, totalRegistros);
    free(direccion);

    if (transaccionGlobal == 0)
      tpcommit(0L);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Eliminar Direccion                                         */
/* Autor   : A.B.G.M.                     Fecha: 21-09-99               */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmDirEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliDIRECCION direccion;
   int totalRegistros=0, i, largoRetorno;
   char cadena[201];
   tCliREGMOD  regModificacion;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&direccion.rutCliente, 0);
   Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccion.tipoDireccion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar direccion empresa - Error: Fallo al conectarse.", 0);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

   sts = SqlCliRecuperarUnaDireccionEmpresa(&direccion);
   if (sts != SQL_SUCCESS)
   {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar direccion empresa - Error: Fallo al recuperar direccion.", 0);

       if (transaccionGlobal == 0)
          tpabort(0L);

       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      sts = SqlCliEliminarDireccionEmpresa(direccion);

      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar direccion empresa - Error: Fallo al eliminar direccion.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);

        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaDireccion(direccion, cadena);
         regModificacion.tipoModificacion = CLI_ELIMINACION_DIRECCION_EMPRESA;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar direccion empresa - Error: Fallo al generar historico.", 0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
           if (transaccionGlobal == 0)
              tpcommit(0L);
           tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }
}

/**************************************************************/
/*************  PARA MANEJO DE TELEFONOS EMPRESA     **********/

/************************************************************************/
/* Objetivo: Crear un Telefono                                          */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsTelEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts,stsAnterior;
   tCliTELEFONO telefono,telAnterior;
   char cadena[201];
   tCliREGMOD  regModificacion;
   int transaccionGlobal;
   int contador=0,i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&telefono.rutCliente, 0);
   telAnterior.rutCliente = telefono.rutCliente;
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Insertar telefonos empresa - Error: Fallo al iniciar la transaccion.",0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   contador = Foccur32(fml,CLI_TIPO_TELEFONO);
   for (i=0; i<contador; i++)
   {
     LlenarRegistroTelefono(fml,&telefono,i);

     telAnterior.tipoTelefono = telefono.tipoTelefono;
     stsAnterior = SqlCliRecuperarUnTelefonoEmpresa(&telAnterior);
     if (stsAnterior == SQL_SUCCESS)
     {
        sts = SqlCliEliminarTelefonoEmpresa(telAnterior);
        if (sts != SQL_SUCCESS)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO,
                "Insertar telefonos empresa - Error: Fallo al eliminar la informacion existente.",0);

            if (transaccionGlobal == 0)
                 tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }

         /*grabar Reg. Modif.*/
         ConcatenaTelefono(telAnterior, cadena);
         regModificacion.tipoModificacion = CLI_MODIFICACION_TELEFONO_EMPRESA;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO,
              "Insertar telefonos empresa - Error: Fallo al generar historico.",0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
     }

     sts = SqlCliCrearTelefonoEmpresa(telefono);

     if (sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se pudo insertar los teléfonos de la empresa.",0);

        if (transaccionGlobal == 0)
           tpabort(0L);

        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
   }

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}


/************************************************************************/
/* Objetivo: Modificar Telefono                                         */
/* Autor   : Claudia Reyes.               Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModTelEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTELEFONO telefono;
   tCliTELEFONO telefonoAnterior;
   char cadena[201];
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroTelefono(fml, &telefono, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar telefonos empresa - Error: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   telefonoAnterior.rutCliente = telefono.rutCliente;
   telefonoAnterior.tipoTelefono = telefono.tipoTelefono;
   sts = SqlCliRecuperarUnTelefonoEmpresa(&telefonoAnterior);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar telefonos empresa - Error: Fallo al recuperar telefono.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarTelefonoEmpresa(telefono);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar telefonos empresa - Error: Fallo al modificar telefonos.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaTelefono(telefonoAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_TELEFONO;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
   }
}


/************************************************************************/
/* Objetivo: Eliminar Telefono                                          */
/* Autor   : Claudia Reyes                Fecha: Julio   1998           */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmTelEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTELEFONO telefono;
   char cadena[201];
   tCliREGMOD  regModificacion;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&telefono.rutCliente, 0);
   Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefono.tipoTelefono, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error: Error al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   sts = SqlCliRecuperarUnTelefonoEmpresa(&telefono);
   if (sts != SQL_SUCCESS)
   {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error: Fallo al recuperar telefono.", 0);

       if (transaccionGlobal == 0)
          tpabort(0L);

       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      sts = SqlCliEliminarTelefonoEmpresa(telefono);
      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error: Fallo al eliminar telefono.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);

        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaTelefono(telefono, cadena);
         regModificacion.tipoModificacion = CLI_ELIMINACION_TELEFONO_EMPRESA;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error: Fallo al generar historico.", 0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
            if (transaccionGlobal == 0)
               tpcommit(0L);
            tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }
}

/************************************************************************/
/* Objetivo: Obtener   Telefonos                                        */
/* Autor   : Claudia Reyes                Fecha: Julio 1998             */
/* Modifica:                              Fecha:                        */
/************************************************************************/

void CliRecTelEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts, grupo, privilegio;
   int totalRegistros=0, i, largoRetorno;
   long   rut;
   short  tipoFono;
   char opcionRecuperacion[2];
   tCliTELEFONO *telefono;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&rut, 0);
   Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error: Fallo al iniciar transaccion.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }


  telefono = (tCliTELEFONO *)calloc(CLI_MAX_TEL, sizeof(tCliTELEFONO));
  if (telefono == NULL)
  {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Eliminar telefonos empresa - Error inesperado en la aplicacion.", 0);
      if (transaccionGlobal == 0)
         tpabort(0L);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
  }

  if (!strncmp ( opcionRecuperacion , CLI_TODOS , 1 ))
  {
    sts=SqlCliRecuperarTelefonosEmpresa(&telefono,rut,CLI_MAX_TEL,&totalRegistros);
  }
  else
  {
     Fget32(fml, CLI_TIPO_TELEFONO, 0,(char *)&tipoFono, 0);
     totalRegistros = 1;
     telefono->rutCliente = rut;
     telefono->tipoTelefono = tipoFono;

     sts = SqlCliRecuperarUnTelefonoEmpresa(telefono);
  }

  if (sts != SQL_SUCCESS)
  {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Recuperar telefonos empresa - Error: Fallo al recuperar telefonos.", 0);

      free(telefono);
      if (transaccionGlobal == 0)
         tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *)tpalloc("FML32", NULL, ESTIMAR_MEMORIA(1 + 22 * totalRegistros, sizeof(int) + sizeof(tCliTELEFONO) * totalRegistros));

   ObtenerRegistroTelefonos(fml, telefono, totalRegistros);
   free(telefono);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera la clave de acceso de un cliente                  */
/* Autor   : Danilo A. Leiva Espinoza                 Fecha: 05-02-2000 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecClaveIvr(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESOIVR clave;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clave.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClaveDeAccesoIvr(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_FECHA , clave.fechaCambioClave, 0);
   Fadd32(fml, CLI_USUARIO, (char *)&clave.ejecutivo, 0);
   Fadd32(fml, CLI_ESTADO_EMISION, (char *)&clave.estadoEmision, 0);
   Fadd32(fml, CLI_FECHA_EMISION, clave.fechaEmision, 0);
   Fadd32(fml, CLI_ESTADO, (char *)&clave.estado, 0);
   Fadd32(fml, CLI_DESCRIPCION_ESTADO, (char *)&clave.descripcionEstado, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Preparar la emisión de la clave de acceso de un cliente    */
/* Autor   : Danilo A. Leiva Espinoza                 Fecha: 24-05-2000 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliActEmiClave(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESOIVR clave;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clave.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClaveDeAccesoIvr(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (clave.estadoEmision != CLI_CLAVE_EMISION_EN_ESPERA)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "La condición actual de emisión de la clave de acceso no permite una reemisión", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_INFORMATION, rqst->name),(char *)fml, 0L, 0L);
   }

   clave.estadoEmision = CLI_CLAVE_EMISION_POR_EMITIR;

   sts = SqlCliModificarClaveDeAccesoEstadoEmision(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al activar la emisión de la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Confirmar la emisión de la clave de acceso de un cliente   */
/* Autor   : Danilo A. Leiva Espinoza                 Fecha: 24-05-2000 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliConEmiClave(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESOIVR clave;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clave.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarClaveDeAccesoEmitir(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al actualizar la emisión de la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/***********************************************************************************/
/* Objetivo: Recuperar Notas de Clasificacion del cliente                          */
/* Autor   : Juan Carlos Meza Garrido                 Fecha: 31-08-2000            */
/***********************************************************************************/
void CliLstClasifCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal, i;
   long sts;
   Q_HANDLE *lista;
   tCliCLASIFICACIONCLIENTE *clasificacionCliente;
   long rutCliente;
   short codigoConsulta=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);

   if (Foccur32(fml, CLI_CODIGO) > 0)
       Fget32(fml, CLI_CODIGO, 0, (char *)&codigoConsulta, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) != NULL)
   {
      if (codigoConsulta == 0)
         sts = SqlCliRecuperarClasificacionClientePorCedula(rutCliente, lista);
      else
         sts = SqlCliRecuperarClasificacionClienteTodasDescentente(rutCliente, lista);
   }
   else
      sts = SQL_MEMORY;

   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar historia de clasificaciones del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(11 * lista->count,
             sizeof(tCliCLASIFICACIONCLIENTE) * lista->count));

   for (i=0; i < lista->count ; i++)
   {
      if (i<12)
      {
        clasificacionCliente = (tCliCLASIFICACIONCLIENTE *)QGetItem(lista,i);
        Fadd32(fml, CLI_FECHA_CLASIFICACION, clasificacionCliente->fechaClasificacion, 0);
        Fadd32(fml, CLI_NOTA_CLIENTE, (char *)&clasificacionCliente->notaCliente, 0);
      }
   }
   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Calcular Clasificacion del Cliente                         */
/* Autor   : Consuelo Montenegro.         Fecha: Octubre   2000         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliCalcularNota(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   char mensajeError[500];
   int  transaccionGlobal;
   long sts;
   Q_HANDLE *operacionCuotasLCC, *operacionCuotasLCD, *operacionCuotasCRC, *morasCuotasOperacion;
   Q_HANDLE *operacionesLCC, *operacionesLCD, *operacionesCRC;
   Q_HANDLE *operaciones;
   tCliOPERACIONES *operacion;
   long     rutCliente;
   short    j=0, notaCliente=NOTA_CLIENTE_NUEVO;
   int      diasMaximaMora=0;
   int      diasMaximaMoraLCC=0;
   int      diasMaximaMoraLCD=0;
   int      diasMaximaMoraCRC=0;
   int      diasMorasCalculado=0;


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********* ContinuidadHoraria [FIN] *********/


   /*------------------------------*/
   /* INICIALIZO LISTAS DE TRABAJO */
   /*------------------------------*/
   operacionesLCC = (Q_HANDLE *)QNew();
   operacionesLCD = (Q_HANDLE *)QNew();
   operacionesCRC = (Q_HANDLE *)QNew();

   /*-------------------------------*/
   /*  RECUPERA OPERACIONES CLIENTE */
   /*-------------------------------*/
   SQLCliRecuperarOperacionesClienteLCC(rutCliente, gFechaProceso, operacionesLCC);
   SQLCliRecuperarOperacionesClienteLCD(rutCliente, gFechaProceso, operacionesLCD);
   SQLCliRecuperarOperacionesClienteCRC(rutCliente, gFechaProceso, operacionesCRC);

   /*-------------------------------------------------*/
   /* Determino nota promedio para tipo operacion LCC */
   /*-------------------------------------------------*/
   diasMaximaMoraLCC = DIAS_MORAS_INVALIDO;
   for (j=0; j<operacionesLCC->count; j++)
   {
        operacion = (tCliOPERACIONES *)QGetItem(operacionesLCC,j);
        morasCuotasOperacion = (Q_HANDLE *)QNew();

        SQLCliRecuperarMorasDeCuotasOperacionesLCC(operacion->credito,operacion->correlativo,
                                                   gFechaProceso,morasCuotasOperacion);

        if (morasCuotasOperacion->count >0)
            diasMorasCalculado = CalcularMoraPromedio (morasCuotasOperacion);
        else
            diasMorasCalculado = DIAS_MORAS_INVALIDO;

        if (diasMorasCalculado > diasMaximaMoraLCC)
            diasMaximaMoraLCC = diasMorasCalculado;

        QDelete(morasCuotasOperacion);
   }

   /*-------------------------------------------------*/
   /* Determino nota promedio para tipo operacion LCD */
   /*-------------------------------------------------*/
   diasMaximaMoraLCD = DIAS_MORAS_INVALIDO;
   for (j=0; j<operacionesLCD->count; j++)
   {
        operacion = (tCliOPERACIONES *)QGetItem(operacionesLCD,j);
        morasCuotasOperacion = (Q_HANDLE *)QNew();

        SQLCliRecuperarMorasDeCuotasOperacionesLCD(operacion->credito,operacion->correlativo,
                                                   gFechaProceso,morasCuotasOperacion);

        if (morasCuotasOperacion->count >0)
            diasMorasCalculado = CalcularMoraPromedio (morasCuotasOperacion);
        else
            diasMorasCalculado = DIAS_MORAS_INVALIDO;

        if (diasMorasCalculado > diasMaximaMoraLCD)
            diasMaximaMoraLCD = diasMorasCalculado;

        QDelete(morasCuotasOperacion);
   }

   /*-------------------------------------------------*/
   /* Determino nota promedio para tipo operacion CRC */
   /*-------------------------------------------------*/
   diasMaximaMoraCRC = DIAS_MORAS_INVALIDO;
   for (j=0; j<operacionesCRC->count; j++)
   {
        operacion = (tCliOPERACIONES *)QGetItem(operacionesCRC,j);
        morasCuotasOperacion = (Q_HANDLE *)QNew();

        SQLCliRecuperarMorasDeCuotasOperacionesCRC(operacion->credito,gFechaProceso,morasCuotasOperacion);

        if (morasCuotasOperacion->count >0)
            diasMorasCalculado = CalcularMoraPromedio (morasCuotasOperacion);
        else
            diasMorasCalculado = DIAS_MORAS_INVALIDO;

        if (diasMorasCalculado > diasMaximaMoraCRC)
            diasMaximaMoraCRC = diasMorasCalculado;

        QDelete(morasCuotasOperacion);
   }

   diasMaximaMora = diasMaximaMoraLCC;
   if (diasMaximaMora < diasMaximaMoraLCD) diasMaximaMora = diasMaximaMoraLCD;
   if (diasMaximaMora < diasMaximaMoraCRC) diasMaximaMora = diasMaximaMoraCRC;

   if (diasMaximaMora !=  DIAS_MORAS_INVALIDO) SQLCliCalcularNotaCliente (diasMaximaMora, &notaCliente);

   fml = (FBFR32 *) tpalloc("FML32", NULL, 512);
   Fadd32(fml, CLI_NOTA_CLIENTE, (char *)&notaCliente, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Objetivo: Recuperar documentacion firmada                                       */
/* Autor   : Andrés Inzunza M.                        Fecha: 27-09-2001            */
/***********************************************************************************/
void CliRecDocFirmad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal, i;
   long sts;
   Q_HANDLE *lista;
   tCliDOCUMENTACIONFIRMADAAUX documentacionFirmada;
   tCliDOCUMENTACIONFIRMADAAUX *pDocumentacionFirmada;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&documentacionFirmada.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name, "CliRecMandato") == 0)
      documentacionFirmada.tipoDocumento = MANDATO_ENROLAMIENTO;
   else
   {
      if (Foccur32(fml, CLI_TIPO_DOCUMENTO) > 0)
         Fget32(fml, CLI_TIPO_DOCUMENTO, 0, (char *)&documentacionFirmada.tipoDocumento, 0);
   } 

   if (documentacionFirmada.tipoDocumento > 0)
   {
      sts = SqlCliRecDocumentacionFirmadaXRutTipoDir(&documentacionFirmada);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe registro de documentación firmada para este cliente", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar documentación firmada", 0);

         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      } 

      Fadd32(fml, CLI_DESCRIPCION, documentacionFirmada.descripcionTipoDocumento, 0);
      Fadd32(fml, CLI_SE_COBRO_GASTO, documentacionFirmada.seCobroGastoNotarial, 0);
   }
   else
   {
      if ((lista = QNew_(10,10)) != NULL)
         sts = SqlCliRecuperarDocumentacionFirmadaTodosXRut(&documentacionFirmada, lista);
      else
         sts = SQL_MEMORY;

      if (sts != SQL_SUCCESS)
      {  
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe registro de documentación firmada para este cliente", 0);
         else 
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar documentación firmada", 0);

         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }

      fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3 * lista->count, sizeof(tCliDOCUMENTACIONFIRMADAAUX) * lista->count));

      for (i=0; i < lista->count ; i++)
      {
         pDocumentacionFirmada = (tCliDOCUMENTACIONFIRMADAAUX *)QGetItem(lista,i);

         Fadd32(fml, CLI_TIPO_DOCUMENTO, (char *)&pDocumentacionFirmada->tipoDocumento, 0);
         Fadd32(fml, CLI_DESCRIPCION, pDocumentacionFirmada->descripcionTipoDocumento, 0);
         Fadd32(fml, CLI_SE_COBRO_GASTO, pDocumentacionFirmada->seCobroGastoNotarial, 0);
      }
      QDelete(lista);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Objetivo: Deja como pagada ("S") el campo seCobroGasto en DocumentacionFirmada  */
/* Autor   : Andrés Inzunza M.                        Fecha: 01-10-2001            */
/***********************************************************************************/
void CliPagDocFirmad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal, i;
   long sts;
   tCliDOCUMENTACIONFIRMADAAUX documentacionFirmada;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&documentacionFirmada.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name, "CliPagMandato") == 0)
      documentacionFirmada.tipoDocumento = MANDATO_ENROLAMIENTO;
   else
      Fget32(fml, CLI_TIPO_DOCUMENTO, 0, (char *)&documentacionFirmada.tipoDocumento, 0);

   sts = SqlCliRecDocumentacionFirmadaXRutTipoDir(&documentacionFirmada);
   if (sts != SQL_SUCCESS)
   { 
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Gasto notarial no se encuentra", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar gasto notarial", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

  /* Ini Pamela Modif 10-2006*/

 /* 

   if (strcmp(documentacionFirmada.seCobroGastoNotarial, "S") == 0)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Gasto notarial ya se encuentra pagado", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_INFORMATION,(char *)fml, 0L, 0L);
   }

 */

 /* Fin Pamela Modif 10-2006*/

   strcpy(documentacionFirmada.seCobroGastoNotarial, "S");

   sts = SqlCliModificaPagoGastoNotarialDocumentacionFirmada(&documentacionFirmada);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo pagar gasto notarial en documentación firmada", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Objetivo: Anular el pago del mandato enrolamiento en tabla DocumentacionFirmada */
/* Autor   : Andrés Inzunza M.                        Fecha: 11-10-2001            */
/***********************************************************************************/
void CliAnuDocFirmad(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal, i;
   long sts;
   tCliDOCUMENTACIONFIRMADAAUX documentacionFirmada;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&documentacionFirmada.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp(rqst->name, "CliAnuPagMandat") == 0)
      documentacionFirmada.tipoDocumento = MANDATO_ENROLAMIENTO;
   else
      Fget32(fml, CLI_TIPO_DOCUMENTO, 0, (char *)&documentacionFirmada.tipoDocumento, 0);

   sts = SqlCliRecDocumentacionFirmadaXRutTipoDir(&documentacionFirmada);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Gasto notarial no se encuentra", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar gasto notarial", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(documentacionFirmada.seCobroGastoNotarial, "N") == 0)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No es posible anular pues gasto notarial no se encuentra pagado", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_INFORMATION,(char *)fml, 0L, 0L);
   }

   strcpy(documentacionFirmada.seCobroGastoNotarial, "N");

   sts = SqlCliModificaPagoGastoNotarialDocumentacionFirmada(&documentacionFirmada);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo anular el pago del gasto notarial en documentación firmada", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Objetivo: Recuperar Ultima clasificación del cliente                            */
/* Autor   : Consuelo Montengro (Omega)               Fecha: 16-01-2001            */
/***********************************************************************************/
void CliRecUltClasif(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal, i;
   long sts;
   tCliCLASIFICACIONCLIENTE clasificacionCliente;
   long rutCliente;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clasificacionCliente.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarUltimaClasificacionCliente(&clasificacionCliente);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar ultima clasificacion del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(5, sizeof(tCliCLASIFICACIONCLIENTE)));

   Fadd32(fml, CLI_FECHA_CLASIFICACION, clasificacionCliente.fechaClasificacion, 0);
   Fadd32(fml, CLI_NOTA_CLIENTE, (char *)&clasificacionCliente.notaCliente, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***********************************************************************************/
/* Objetivo: Recuperar comportamiento interno pago                                 */
/* Autor   : Consuelo Montengro (Omega)               Fecha: 21-01-2001            */
/***********************************************************************************/
void CliRecCompInter(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   Q_HANDLE *lista;
   int transaccionGlobal, i, totalRegistros;
   long sts;
   tCliCOMPORTAMIENTOINTERNO *comportamientoInternoPago;

   /****** Buffer de entrada *****/
   fml = (FBFR32 *)rqst->data;

   /****** Cuerpo del servicio ******/
   transaccionGlobal=tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarComportamientoInternoPagoTodos(lista);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar comportamiento interno pago", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3 * lista->count, sizeof(tCliCOMPORTAMIENTOINTERNO) * lista->count));

   for (i=0; i < lista->count ; i++)
   {
      comportamientoInternoPago = (tCliCOMPORTAMIENTOINTERNO *)QGetItem(lista,i);
      Fadd32(fml, CLI_NOTA_CLIENTE, (char *)&comportamientoInternoPago->nota, 0);
      Fadd32(fml, CLI_TABLA_DESCRIPCION, comportamientoInternoPago->descripcionNota,0);
   }
   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/*********************************************************************************/
/* Objetivo: Modificar la clave de acceso Internet de un cliente                 */
/* Objetivo: Crea por primera vez, la clave de acceso Internet de un cliente     */
/* Autor   : Boris Contreras Mac-lean                        Fecha: 08-05-2002   */
/* Modifica:                                                 Fecha:              */
/*********************************************************************************/
void CliModClaveInt(TPSVCINFO *rqst)
{ 
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESOINTERNET clave;
   tCliCLAVEDEACCESOINTERNET claveConsulta;
   tCliREGMOD registroModificacion;
   long rut;
   long horaLng;
   short sucursal=1;
   char pinUsuario[CLI_LARGO_PIN_INTERNET + 1];
   char nuevoPinUsuario[CLI_LARGO_PIN_INTERNET + 1];
   char tempNuevoPinUsuario[20 + 1];     /*--  En caso de ingresar una clave mayor que CLI_LARGO_PIN_IVR --*/
   char mensaje[250 + 1];
   char fechaConsulta[8 + 1];
   tCliSOLICITUDCAMBIOCLAVE solicitud;

/* lo nuevo OCT-2011 JHBC */

   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaCalendario[DIM_FECHA_HORA];
   time_t hora;
   long  identificadorUsuario;
   char  nombreUsuario[15 + 1];

/* fin de lo nuevo OCT-2011 JHBC */

   printf("(CliModClaveInt) inicio \n");

   memset((char *)&solicitud, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));

/* lo nuevo OCT-2011 JHBC */

   memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));

/* fin de lo nuevo OCT-2011 JHBC */

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT,            0, (char *)&rut, 0);

   if (Foccur32(fml, CLI_SUCURSAL) > 0)
      Fget32(fml, CLI_SUCURSAL,       0, (char *)&sucursal, 0);

   if (strcmp(rqst->name, "CliGenClaveInt")==0 || strcmp(rqst->name, "CliGenClvIntInt")==0 )
   {
      Fget32(fml, CLI_NUEVO_PIN_INTERNET,  0, tempNuevoPinUsuario, 0);

      if (Foccur32(fml, CLI_USUARIO) > 0)
         Fget32(fml, CLI_USUARIO,  0, (char *)&clave.ejecutivo, 0);
      else
         clave.ejecutivo = 0;

      clave.estado = CLI_CLAVE_ACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_RECEPCIONADA;
      strcpy(clave.pinOffset, " ");
   }
   else  
   {
      Fget32(fml, CLI_NUEVO_PIN_INTERNET,  0, tempNuevoPinUsuario, 0);
      Fget32(fml, CLI_PIN_INTERNET,        0, pinUsuario, 0);

      clave.ejecutivo = 0;
      clave.estado = CLI_CLAVE_ACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_RECEPCIONADA;
      strcpy(clave.pinOffset, " ");
   }

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/******** ContinuidadHoraria [INI] ********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/******** ContinuidadHoraria [FIN] ********/


   /*-- Se debe validar que el largo de la clave ingresada corresponda a la solicitada --*/
   if (strlen(tempNuevoPinUsuario) != CLI_LARGO_PIN_INTERNET)
   { 
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el formato de su clave.", 0); 
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CLI_LARGO_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
   }
   
   /*-- Se traspasa el valor de la variable temporal a la variable definitiva, con el largo correspondiente --*/
   strncpy(nuevoPinUsuario, tempNuevoPinUsuario, CLI_LARGO_PIN_INTERNET);
   nuevoPinUsuario[CLI_LARGO_PIN_INTERNET] = 0;

   claveConsulta.rutCliente = rut;
   strcpy(claveConsulta.pinOffset, " ");

   sts = SqlCliRecuperarClaveDeAccesoInternet(&claveConsulta);
   if (sts != SQL_SUCCESS)
   {
      if (sts != SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

   if (strcmp(rqst->name, "CliGenClaveInt")==0 || strcmp(rqst->name, "CliGenClvIntInt")==0 )
   {
      if (strcmp(rqst->name, "CliGenClaveInt") == 0)
      { 
         /************************************************************************/
         /* Recuperacion de una solicitud en estado creada para fecha calendario */
         /************************************************************************/
         solicitud.rutCliente = rut;
         solicitud.tipoCambioClave = CAMBIO_CLAVE_INTERNET;
         solicitud.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;

         strncpy(fechaConsulta, gFechaCalendario, 8);
         fechaConsulta[8] = '\0';

         sts = SqlCliRecuperarSolicitudCambioClave(fechaConsulta, &solicitud);
         if (sts != SQL_SUCCESS)
         {
            if (sts == SQL_NOT_FOUND)
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Se requiere una solicitud de cambio de clave", 0);
            else
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar solicitud de cambio clave", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
      }
   }
   else
   {
      sts = ValidarPinInternet(rut, pinUsuario, sucursal, mensaje);
      if (sts != SRV_SUCCESS && sts != SRV_CLI_MODIFICAR_PIN)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, mensaje, 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, sts, (char *)fml, 0L, 0L);
      }
   }

   /*-- Ya sea para la nueva clave o para la modificada, se debe recuperar el Pin Offset --*/
   sts = RecuperarPinOffsetInternet(rut, nuevoPinUsuario, clave.pinOffset, mensaje);
   if (sts != SRV_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, mensaje, 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, sts, (char *)fml, 0L, 0L);
   }

   clave.rutCliente = rut;
   clave.intentosFallidos = 0;

   sts = SqlCliModificarClaveDeAccesoInternet(clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
        registroModificacion.tipoModificacion = CLI_GENERACION_CLAVE_INTERNET;
        sprintf(registroModificacion.modificacion,"%-18s", clave.pinOffset);

        sts = SqlCliInsertarClaveDeAccesoInternet(clave);

        if (sts != SQL_SUCCESS)
        {
           if (sts == SQL_DUPLICATE_KEY)
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente ya posee clave de acceso", 0);
           else
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar la clave de acceso del cliente", 0);
           TRX_ABORT(transaccionGlobal);
           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }

      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar la clave de acceso del cliente", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   else
   {
      registroModificacion.tipoModificacion = CLI_MODIFICACION_CLAVE_INTERNET;
      sprintf(registroModificacion.modificacion,"%-18s %-18s", claveConsulta.pinOffset, clave.pinOffset);
   }

   registroModificacion.rutCliente = clave.rutCliente;
   registroModificacion.usuario = (clave.ejecutivo == 0)? 1:clave.ejecutivo;
   registroModificacion.sucursal = sucursal;
   strcpy(registroModificacion.fecha, gFechaProceso);

   sts = SqlCliCrearRegMod(registroModificacion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al registrar modificación de datos del cliente.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(rqst->name, "CliGenClaveInt")==0)
   {
      strncpy(solicitud.fechaHora,gFechaCalendario, 8);
      solicitud.fechaHora[8] = '\0';
      sts = SqlCliModificarEstadoSolicitudCambioClave(ESTADO_SOLICITUD_CAMBIO_CLAVE_UTILIZADA, solicitud);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar solicitud de cambio clave", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

/* lo nuevo OCT-2011 JHBC */
if ( strcmp(rqst->name, "CliGenClvIntInt")==0 )
{
      OmRecuperarIpServ(bufferSysLog.ipDestino);
      bufferSysLog.ipDestino[15] = 0;
      identificadorUsuario = (clave.ejecutivo == 0)? 1:clave.ejecutivo;

      sts = OmRecNombreUsuarioHrm(identificadorUsuario,nombreUsuario);
      if(sts != SRV_SUCCESS)
      {
         printf("Fallo al recuperar el nombre con funcion OmRecNombreUsuarioHrm identificador=[%d]\n",identificadorUsuario);
      }
      printf("(CliModClaveInt) nombreUsuario =[%s]\n",nombreUsuario);

      strncpy(bufferSysLog.usuarioDeAplicacion, nombreUsuario,15);
      bufferSysLog.usuarioDeAplicacion[15] = 0;

      strcpy(bufferSysLog.ipOrigen," ");
      sprintf(bufferSysLog.sRut,"%08li-%c",rut,OmCalcularDigitoVerificador(rut));

      bufferSysLog.usuarioDeRed[0] = 0;
      bufferSysLog.codigoSistema = CLI_SISTEMA_ADMINISTRACION_CLIENTES;
      hora = (time_t)time((time_t *)NULL);
      strftime(fechaCalendario, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
      strcpy(bufferSysLog.fechaHora,fechaCalendario);

/* JHBC - INI 2012 */

       strcpy(bufferSysLog.nombreDeAplicacion,CLI_CANAL_INTERNET);

/* JHBC - FIN 2012 */

      /* Escribir en el SYSLOG.LOG */
      OmEscribirEnSysLog(&bufferSysLog);
      /* Inserción del log a una tabla temporal */
      sts = OmGrabarReg_SysLog(&bufferSysLog);
      if(sts != SRV_SUCCESS)
      {
         printf("Error al insertar en la tabla temporal REG_SYSLOG.(%s/%li/%s)\n",bufferSysLog.fechaHora ,bufferSysLog.codigoSistema ,bufferSysLog.usuarioDeRed);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al grabar registro REG_SYSLOG EN BD HERMES", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
}

/* fin de lo nuevo OCT-2011 JHBC */


   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************************************************************************/
/* Objetivo: Modificar la clave de acceso Ivr de un cliente                 */
/* Objetivo: Crea por primera vez, la clave de acceso Ivr de un cliente     */
/* Autor   : Boris Contreras Mac-lean                   Fecha: 08-05-2002   */
/* Modifica:                                            Fecha:              */
/****************************************************************************/
void CliModClaveIvr(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int   transaccionGlobal;
   long  sts;
   tCliCLAVEDEACCESOIVR clave;
   tCliCLAVEDEACCESOIVR claveConsulta;
   tCliREGMOD registroModificacion;
   tCliSOLICITUDCAMBIOCLAVE solicitud;
   char fechaConsulta[8 + 1];
   long  rut;
   short sucursal=1;
   char  tempNuevoPin[20 + 1];     /*--  En caso de ingresar una clave mayor que CLI_LARGO_PIN_IVR --*/
   char  pinUsuario[CLI_LARGO_PIN_IVR + 1];
   char  nuevoPinUsuario[CLI_LARGO_PIN_IVR + 1];
   char  mensaje[250 + 1];
   char  *temp=NULL;
   short tema; 

/* lo nuevo OCT-2011 JHBC */
   long codigoSistema;
   tHrmBufferTrans  bufferSysLog;
   char   fechaCalendario[DIM_FECHA_HORA];
   time_t hora;
   long   identificadorUsuario;
   char   nombreUsuario[15 + 1];
/* fin de lo nuevo OCT-2011 JHBC */

   /* TVT. 16-10-2009. Cambio de Pin en Claro. In. */
   char pinOffsetS[4+1];
   char Respuesta[LARGO_RESPUESTA];
   char nroTarjetaAux[20 + 1];
   char numeroTarjeta[16 + 1];
   char campoSw[2+1];
   char campoHw[2+1];
   int ResultadoCambioPin=0;
   long status=0; 
   char test[15];

   strcpy(campoSw,"SW");
   strcpy(campoHw,"HW");
   memset(pinOffsetS, 0, sizeof(pinOffsetS));
   
   /* TVT. 16-10-2009. Cambio dePin en Claro. Fn. */

/* lo nuevo OCT-2011 JHBC */
   memset(&bufferSysLog,0,sizeof(tHrmBufferTrans));
/* fin de lo nuevo OCT-2011 JHBC */

   

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);

   if (Foccur32(fml, CLI_SUCURSAL) > 0)
       Fget32(fml, CLI_SUCURSAL, 0, (char *)&sucursal, 0);

   if (strcmp(rqst->name, "CliGenClaveIvr") == 0 || strcmp(rqst->name, "CliGenClvIvrIvr") == 0)
   {
      Fget32(fml, CLI_NUEVO_PIN_IVR, 0, tempNuevoPin, 0);

      if (Foccur32(fml, CLI_USUARIO) > 0)
         Fget32(fml, CLI_USUARIO, 0, (char *)&clave.ejecutivo, 0);
      else
         clave.ejecutivo = 0;

      clave.estado = CLI_CLAVE_ACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_RECEPCIONADA;
      clave.pinOffset = 0;
   }
   else
   {
      /*-- Se modifica la clave por el usuario --*/

      Fget32(fml, CLI_NUEVO_PIN_IVR,  0, tempNuevoPin, 0);
      Fget32(fml, CLI_PIN_IVR,        0, pinUsuario, 0);

      clave.ejecutivo = 0;
      clave.estado = CLI_CLAVE_ACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_RECEPCIONADA;
      clave.pinOffset = 0;
   }

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********** ContinuidadHoraria [INI] **********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********** ContinuidadHoraria [FIN] **********/

   /*-- Se debe validar que el largo de la clave ingresada corresponda a la solicitada --*/
   if (strlen(tempNuevoPin) != CLI_LARGO_PIN_IVR)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Su nueva clave debe poseer 4 dígitos.", 0);  
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SRV_CLI_LARGO_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
   }

   /*-- Se traspasa el valor de la variable temporal a la definitiva, con el largo que corresponde --*/
   strncpy(nuevoPinUsuario, tempNuevoPin, CLI_LARGO_PIN_IVR);
   nuevoPinUsuario[CLI_LARGO_PIN_IVR] = 0;

   claveConsulta.rutCliente = rut;
   claveConsulta.pinOffset = 0 ;

   sts = SqlCliRecuperarClaveDeAccesoIvr(&claveConsulta);
   if (sts != SQL_SUCCESS)
   {
      if (sts != SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
      else
      {
        if (strcmp(rqst->name, "CliModClaveIvr")==0)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "No posee clave de acceso", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }
      }
   }

   if (strcmp(rqst->name, "CliGenClaveIvr") == 0 || strcmp(rqst->name, "CliGenClvIvrIvr") == 0)
   {
      if (strcmp(rqst->name, "CliGenClaveIvr") == 0)
      {
         solicitud.rutCliente = rut;
         solicitud.tipoCambioClave = CAMBIO_CLAVE_IVR;
         solicitud.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;

         strncpy(fechaConsulta, gFechaCalendario, 8);
         fechaConsulta[8] = '\0';

         sts = SqlCliRecuperarSolicitudCambioClave(fechaConsulta, &solicitud);

         if (sts != SQL_SUCCESS)
         {
            if (sts == SQL_NOT_FOUND)
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Se requiere una solicitud de cambio de clave", 0);
            else
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar solicitud de cambio clave", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
      }
   }
   else
   {
	/* TVT. 16-10-2009. Validacion de Pin en Claro. In. */
	if (retornoTipoValidar(campoSw))             /* Si Validacion Software = 1 */
	{
      		/*-- Se debe validar la clave antigua antes de ser modificada.--*/
      		sts = ValidarPinIvr(rut, pinUsuario, sucursal, mensaje);
      		if (sts != SRV_SUCCESS && sts != SRV_CLI_MODIFICAR_PIN)
      		{
           		Fadd32(fml, HRM_MENSAJE_SERVICIO, mensaje, 0);
           		TRX_ABORT(transaccionGlobal);
           		tpreturn(TPFAIL, sts, (char *)fml, 0L, 0L);
      		}
	}
   }

   /* TVT. 16-10-2009. Cambio de Pin en Claro. In. */
   if (retornoTipoValidar(campoSw))		/* Si Validacion Software = 1 */
   {
	   sts = RecuperarPinOffsetIvr(rut, nuevoPinUsuario, &clave.pinOffset, mensaje);
	   if (sts != SRV_SUCCESS)
	   {
	      Fadd32(fml, HRM_MENSAJE_SERVICIO, mensaje, 0);
	      TRX_ABORT(transaccionGlobal);
	      tpreturn(TPFAIL, sts, (char *)fml, 0L, 0L);
	   }
   }
   /* TVT. 16-10-2009. Cambio de Pin en Claro. Fn. */

   clave.rutCliente = rut;
   clave.intentosFallidos = 0;

   /* TVT. 16-10-2009. Cambio de Pin en Claro. In. */
   if (retornoTipoValidar(campoHw))		/* Si Validacion Hardware = 1 */
   {
      if (strcmp(rqst->name, "CliGenClaveIvr") == 0 || strcmp(rqst->name, "CliGenClvIvrIvr") == 0)
      {
         sprintf((char *)pinUsuario, "NULL");
         strcpy( pinOffsetS, "0000" );
      }
      else
      {
         sprintf(pinOffsetS,"%04d",claveConsulta.pinOffset);

         /*-- Se valida que el estado del offset o clave, no se encuentre bloqueada --*/
         if (claveConsulta.estado == TCR_CLAVE_BLOQUEADA)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Su clave ha sido bloqueada, contáctese con su ejecutivo de cuenta", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL,SRV_TCR_PIN_BLOQUEADO, (char *)fml, 0L, 0L);
         }
      }
	
      	memset(nroTarjetaAux, 0, 20);
      	strcpy(nroTarjetaAux, "0000");
      /*	strncat(nroTarjetaAux, numeroTarjeta + strlen(numeroTarjeta) - 13, 12);  */
      	ObtenerRutIvr(rut, nroTarjetaAux);

  	ResultadoCambioPin = CambiarPinF2(pinUsuario,nuevoPinUsuario,pinOffsetS,nroTarjetaAux,nroTarjetaAux,BANCA_TELEFONICA_F2,Respuesta);

	printf("\nResultadoCambioPin es %d", ResultadoCambioPin);
	if (ResultadoCambioPin != CODE_CSIN_PIN_ANTIGUO && ResultadoCambioPin != CODE_CPIN_VERIFICADO )
        {
                switch(ResultadoCambioPin)
                {
                        case CODE_CLARGO_PIN_NUEVO_NO_VALIDO    :
                                printf("Su nueva clave debe poseer 4 dígitos.\n");
                                printf("ERROR Validacion por Hardware: RETORNO [%d] [%s]", ResultadoCambioPin, Respuesta);
                                status = SRV_CLI_LARGO_PIN_NO_VALIDO;
                        break;
                        case CODE_CPIN_ANTIGUO_NO_VERIFICADO    :
                        case CODE_CLARGO_PIN_ANTIGUO_NO_VALIDO  :
                        case CODE_CSIN_PIN_ANTIGUO      :
                                sts = PIN_CLINOOK(&clave);
                                if (sts != SRV_SUCCESS)
                                {
                                        if (sts == SRV_FOUND) {
                                                printf("cambiarPinIvr. Ivr.Realiza commit.\n");
                                                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clave de acceso inválida", 0);
                                                TRX_COMMIT(transaccionGlobal);
                                                tpreturn(TPSUCCESS, SRV_CLI_PIN_NO_VALIDO, (char *)fml, 0L, 0L);
                                        } else {
                                                printf("cambiarPinIvr..Falla cambiarPinIvr.\n");
                                        }
                                }
                                status = SRV_CLI_PIN_NO_VALIDO;
                        break;
                        case CODE_CERROR_PROCESAR_PIN_ANTIGUO   :
                        case CODE_CERROR_PROCESAR_PIN_NUEVO     :
                        case CODE_CERROR_DESCONOCIDO    :
                        case SC_RET_CLIENT_TIMEOUT  :
                        case SC_RET_SERVER_TIMEOUT  :
                        case SC_RET_CLIENT_COMM_ERR :
                        case SC_RET_SERVER_COMM_ERR :
                        case SC_RET_GENERAL_ERROR   :
                        case SC_RET_INVALID_COMMAND :
                        case SC_RET_FEW_PARAMS      :
                        case SC_RET_INVALID_PARAM   :
                                printf("\nERROR Validacion por Hardware: RETORNO [%d] [%s]", ResultadoCambioPin, Respuesta);
                                status = SRV_CRITICAL_ERROR;
                        break;
                        default:
                        break;
                }
        }
        else
        {
                clave.pinOffset = atoi(pinOffsetS);
   		sts = PIN_CLIOK(&clave);
   		if (sts != SRV_SUCCESS)
   		{
      			printf("cambiarPinIvr..Falla cambiarPinIvr.\n");
        		status = SRV_CRITICAL_ERROR;
   		}
                status = SRV_SUCCESS;
        }

        /* Validacion por HARDWARE */
        /************ Modificacion por Encrptacion *************/

        if ( status != SRV_SUCCESS )
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, Respuesta, 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }

   }

   /*-- Se ingresa los nuevos datos --*/
   sts = SqlCliModificarClaveDeAccesoIvr(clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
        registroModificacion.tipoModificacion = CLI_GENERACION_CLAVE_IVR;
        sprintf(registroModificacion.modificacion,"%04d", clave.pinOffset);
	
        sts = SqlCliInsertarClaveDeAccesoIvr(clave);
	
        if (sts != SQL_SUCCESS)
        {
           if (sts == SQL_DUPLICATE_KEY)
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente ya posee clave de acceso", 0);
           else
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar la clave de acceso del cliente", 0);
           TRX_ABORT(transaccionGlobal);
           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar la clave de acceso del cliente.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   else
   {
      registroModificacion.tipoModificacion = CLI_MODIFICACION_CLAVE_IVR;
      sprintf(registroModificacion.modificacion,"%04d %04d", claveConsulta.pinOffset, clave.pinOffset);
   }
	 
   registroModificacion.rutCliente = clave.rutCliente;
   registroModificacion.usuario = (clave.ejecutivo == 0)? 1:clave.ejecutivo;
   registroModificacion.sucursal = sucursal;
   strcpy(registroModificacion.fecha, gFechaProceso);

   sts = SqlCliCrearRegMod(registroModificacion);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al registrar modificación de datos del cliente.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(rqst->name, "CliGenClaveIvr")==0)
   {
      strncpy(solicitud.fechaHora,gFechaCalendario, 8);
      solicitud.fechaHora[8] = '\0';
      sts = SqlCliModificarEstadoSolicitudCambioClave(ESTADO_SOLICITUD_CAMBIO_CLAVE_UTILIZADA, solicitud);

      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar solicitud de cambio clave", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

/* lo nuevo OCT-2011 JHBC */
   if ( strcmp(rqst->name, "CliGenClvIvrIvr")==0 )
   {
      OmRecuperarIpServ(bufferSysLog.ipDestino);
      bufferSysLog.ipDestino[15] = 0;
      identificadorUsuario = (clave.ejecutivo == 0)? 1:clave.ejecutivo;
      sts = OmRecNombreUsuarioHrm(identificadorUsuario,nombreUsuario);
      if(sts != SRV_SUCCESS)
      {
         printf("Fallo al recuperar el nombre con funcion OmRecNombreUsuarioHrm identificador=[%d]\n",identificadorUsuario);
      }

      printf("(CliModClaveIvr) nombreUsuario =[%s]\n",nombreUsuario);

      strncpy(bufferSysLog.usuarioDeAplicacion, nombreUsuario,15);
      bufferSysLog.usuarioDeAplicacion[15] = 0;

      strcpy(bufferSysLog.ipOrigen," ");
      /* sprintf(bufferSysLog.sRut,"%0li",rut); */
      sprintf(bufferSysLog.sRut,"%8li-%c",rut,OmCalcularDigitoVerificador(rut));

      bufferSysLog.usuarioDeRed[0] = 0;
      bufferSysLog.codigoSistema = CLI_SISTEMA_ADMINISTRACION_CLIENTES;
      hora = (time_t)time((time_t *)NULL);
      strftime(fechaCalendario, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
      strcpy(bufferSysLog.fechaHora,fechaCalendario);

/* JHBC - INI 2012 */

       strcpy(bufferSysLog.nombreDeAplicacion,CLI_CANAL_IVR);

/* JHBC - FIN 2012 */

      /* Escribir en el SYSLOG.LOG */
      OmEscribirEnSysLog(&bufferSysLog);
      /* Inserción del log a una tabla temporal */
      sts = OmGrabarReg_SysLog(&bufferSysLog);
      if(sts != SRV_SUCCESS)
      {
         printf("Error al insertar en la tabla temporal REG_SYSLOG.(%s/%li/%s)\n",bufferSysLog.fechaHora,bufferSysLog.codigoSistema ,bufferSysLog.usuarioDeRed);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al grabar registro REG_SYSLOG EN BD HERMES", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }
/* fin de lo nuevo OCT-2011 JHBC */


   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Inserta la solicitud de cambio clave desde cliente         */
/* Autor   : Maria Consuelo Montenegro Fecha: 19-06-2002                */
/* Modifica:                                                            */
/************************************************************************/
void CliInsSolCmbClv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int  transaccionGlobal;
   long sts=0;
   long horaLng;
   tCliSOLICITUDCAMBIOCLAVE solicitud;
   tCliSOLICITUDCAMBIOCLAVE solicitudRecuperada;
   char fechaConsulta[8 + 1];

   memset((char *)&solicitud, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));
   memset((char *)&solicitudRecuperada, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&solicitud.rutCliente, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&solicitud.usuario, 0);
   Fget32(fml, CLI_SUCURSAL, 0, (char *)&solicitud.sucursal, 0);
   Fget32(fml, CLI_CODIGO, 0, (char *)&solicitud.tipoCambioClave, 0);

/***************** ContinuidadHoraria [INI] ********************
   strncpy(solicitud.fechaHora, gFechaCalendario, 8);
   solicitud.fechaHora[8] = '\0';

   horaLng = f_horaComoLong();
   f_strcatlng(solicitud.fechaHora, horaLng, 6);

   strncpy(fechaConsulta, gFechaCalendario, 8);
   fechaConsulta[8] = '\0';
***************** ContinuidadHoraria [FIN] ********************/

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/******** ContinuidadHoraria [INI] ********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("tpsvrinit: Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }

   strncpy(solicitud.fechaHora, gFechaCalendario, 8);
   solicitud.fechaHora[8] = '\0';

   horaLng = f_horaComoLong();
   f_strcatlng(solicitud.fechaHora, horaLng, 6);

   strncpy(fechaConsulta, gFechaCalendario, 8);
   fechaConsulta[8] = '\0';

/******** ContinuidadHoraria [FIN] ********/

   /*************************************************************************/
   /* Modificar solicitud de cambio clave de estado creada a estado anulada */
   /*************************************************************************/
   solicitudRecuperada.rutCliente = solicitud.rutCliente;
   solicitudRecuperada.tipoCambioClave = solicitud.tipoCambioClave;
   solicitudRecuperada.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;
   strcpy(solicitudRecuperada.fechaHoraEstado , solicitud.fechaHora);

   sts = SqlCliModificarEstadoSolicitudCambioClaveTodas(fechaConsulta, ESTADO_SOLICITUD_CAMBIO_CLAVE_ANULADA, solicitudRecuperada);

   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {  
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar solicitud de cambio clave", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   /*******************************************************/
   /* Insertar solicitud de cambio clave en estado creada */
   /*******************************************************/
   solicitud.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;
   sts = SqlCliInsertarSolicitudCambioClave(&solicitud);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente ya cuenta con este tipo de solicitud registrada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo insertar la solicitud del cliente ", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************/
/* Objetivo: Modificacion de solicitud cambio clave de creada a utilizada */
/* Autor   : Maria Consuelo Montenegro Fecha: 05-07-2002                  */
/* Modifica:                                                              */
/**************************************************************************/
void CliModSolCmbClv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int  transaccionGlobal;
   long sts=0;
   long horaLng;
   char fechaConsulta[8 + 1];
   tCliSOLICITUDCAMBIOCLAVE solicitud;
   tCliSOLICITUDCAMBIOCLAVE solicitudRecuperada;

   memset((char *)&solicitud, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));
   memset((char *)&solicitudRecuperada, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&solicitud.rutCliente, 0);
   Fget32(fml, CLI_CODIGO, 0, (char *)&solicitud.tipoCambioClave, 0);

/************** ContinuidadHoraria [INI] *******************
   strncpy(solicitud.fechaHora, gFechaCalendario, 8);
   solicitud.fechaHora[8] = '\0';
*************** ContinuidadHoraria [FIN] *******************/
/*
   horaLng = f_horaComoLong();
   f_strcatlng(solicitud.fechaHora, horaLng, 6);
*/

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/*********** ContinuidadHoraria [INI] ***********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }

   strncpy(solicitud.fechaHora, gFechaCalendario, 8);
   solicitud.fechaHora[8] = '\0';

/*********** ContinuidadHoraria [FIN] ***********/


   /*********************************************************************/
   /* Recuperacion se una solicitud en estado creada para fecha proceso */
   /*********************************************************************/
   solicitudRecuperada.rutCliente = solicitud.rutCliente;
   solicitudRecuperada.tipoCambioClave = solicitud.tipoCambioClave;
   solicitudRecuperada.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;

   strncpy(fechaConsulta, gFechaCalendario, 8);
   fechaConsulta[8] = '\0';

   sts = SqlCliRecuperarSolicitudCambioClave(fechaConsulta, &solicitudRecuperada);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encuentra este tipo de solicitud.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar solicitud de cambio clave", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   /***************************************************************************/
   /* Modificar solicitud de cambio clave de estado creada a estado utilizada */
   /***************************************************************************/
   solicitud.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;

   sts = SqlCliModificarEstadoSolicitudCambioClave(ESTADO_SOLICITUD_CAMBIO_CLAVE_UTILIZADA, solicitud);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar solicitud de cambio clave", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar la solicitud de cambio clave desde cliente       */
/* Autor   : Maria Consuelo Montenegro Fecha: 19-06-2002                */
/* Modifica:                                                            */
/************************************************************************/
void CliRecSolCmbClv(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int      transaccionGlobal;
   long     sts=0;
   char     fechaConsulta[8 + 1];
   tCliSOLICITUDCAMBIOCLAVE solicitud;

   memset((char *)&solicitud, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&solicitud.rutCliente, 0);
   Fget32(fml, CLI_CODIGO, 0, (char *)&solicitud.tipoCambioClave, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********* ContinuidadHoraria [FIN] *********/

   /*********************************************************************/
   /* Recuperacion se una solicitud en estado creada para fecha proceso */
   /*********************************************************************/
   solicitud.estado = ESTADO_SOLICITUD_CAMBIO_CLAVE_CREADA;

   strncpy(fechaConsulta, gFechaCalendario, 8);
   fechaConsulta[8] = '\0';

   sts = SqlCliRecuperarSolicitudCambioClave(fechaConsulta, &solicitud);

   if (sts != SQL_SUCCESS) 
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se encuentra este tipo de solicitud.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar solicitud de cambio clave", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   fml = (FBFR32 *) tpalloc("FML32", NULL,1024);

   Fadd32(fml, CLI_USUARIO, (char *)&solicitud.usuario, 0);
   Fadd32(fml, CLI_SUCURSAL, (char *)&solicitud.sucursal, 0);
   Fadd32(fml, CLI_CODIGO, (char *)&solicitud.tipoCambioClave, 0);
   Fadd32(fml, CLI_FECHA_ACTIVACION, solicitud.fechaHora, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta registro usuario consulta preguntas secretas       */
/* Autor   : Maria Consuelo Montenegro Fecha: 19-06-2002                */
/* Modifica:                                                            */
/************************************************************************/
void CliRegCtrAccEje(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int  transaccionGlobal;
   long sts=0;
   long horaLng;
   tCliSOLICITUDCAMBIOCLAVE registro;

   memset((char *)&registro, 0, sizeof(tCliSOLICITUDCAMBIOCLAVE));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&registro.rutCliente, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&registro.usuario, 0);
   Fget32(fml, CLI_CODIGO_ACCION, 0, (char *)&registro.tipoAccion, 0);
   Fget32(fml, CLI_CODIGO_SISTEMA, 0, (char *)&registro.sistema, 0);

/********** ContinuidadHoraria ********************
   strcpy(registro.fechaHora, gFechaProceso);
   horaLng = f_horaComoLong();
   f_strcatlng(registro.fechaHora, horaLng, 6);
************ ContinuidadHoraria *********************/

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********** ContinuidadHoraria [INI] **********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
   strcpy(registro.fechaHora, gFechaProceso);
   horaLng = f_horaComoLong();
   f_strcatlng(registro.fechaHora, horaLng, 6);

/********** ContinuidadHoraria [FIN] **********/


   sts = SqlCliInsertarRegistroUsuarioConsulta(&registro);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo insertar el registro usuario para control", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*********************************************************************************/
/* Objetivo: modifica datos de la tabla tCliFAX, si no lo encuentra, se inserta. */
/* Autor   : CMCH                                              Fecha: 03-07-2002 */
/* Modifica:                                                   Fecha:            */
/*********************************************************************************/
void CliModFax(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliFAX fax;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX,           0, (char *)&fax.rutCliente,   0);
   Fget32(fml, CLI_TIPO_FAX,       0, (char *)&fax.tipoFax,      0);
   Fget32(fml, CLI_CODIGO_DE_AREA, 0, (char *)&fax.codigoDeArea, 0);
   Fget32(fml, CLI_NUMERO_DE_FAX,  0, (char *)&fax.numeroDeFax,  0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarFax(&fax);
   if (sts != SQL_SUCCESS)
   {
	if (sts != SQL_NOT_FOUND)
        {     
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar fax del cliente", 0);
          TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
	else
        {
           sts = SqlCliInsertarFax(&fax);
           if (sts != SQL_SUCCESS)
           {     
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar fax del cliente", 0);
              TRX_ABORT(transaccionGlobal);
              tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
           }
        }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: recupera datos de la tabla tCliFAX                         */
/* Autor   : CMCH                                     Fecha: 03-07-2002 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecFax(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int       transaccionGlobal;
   long      sts;
   short     tipoFax;
   long      rutCliente;
   int       especificoTipoDeFax;
   int       totalRegistros=0, i=0;
   tCliFAX   *fax;
   Q_HANDLE  *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX,0, (char *)&rutCliente, 0);

   especificoTipoDeFax = Foccur32(fml,  CLI_TIPO_FAX);
   if (especificoTipoDeFax)
      Fget32(fml, CLI_TIPO_FAX,0, (char *)&tipoFax, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew()) == NULL)
   {
      sts = SQL_MEMORY;
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (especificoTipoDeFax)
      sts = SqlCliRecuperarFax(lista, tipoFax, rutCliente);
   else
      sts = SqlCliRecuperarFaxTodos(lista, rutCliente);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen numero de fax del cliente", 0);
      else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar fax del cliente", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL,1024);
 
   for(i=0; i < lista->count ;i++)
   {
      fax = (  tCliFAX *) QGetItem( lista, i);

       Fadd32(fml, CLI_RUTX,           (char *)&fax->rutCliente,    0);
       Fadd32(fml, CLI_TIPO_FAX,       (char *)&fax->tipoFax,      0);
       Fadd32(fml, CLI_CODIGO_DE_AREA, (char *)&fax->codigoDeArea, 0);
       Fadd32(fml, CLI_NUMERO_DE_FAX,  (char *)&fax->numeroDeFax,  0);
   }
   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/*********************************************************************************/
/* Objetivo: Bloqueo de clave IVR por olvido. 					 */
/* Autor   : Jan Riega Z.                                      Fecha: 23-07-2002 */
/* Modifica:                                                   Fecha:            */
/*********************************************************************************/
void CliBloqeoClaIvr(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESOIVR  claveAccesoIvr;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX,0, (char *)&claveAccesoIvr.rutCliente,   0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliBloquearClaveDeAccesoIvr(&claveAccesoIvr);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al bloquear clave Ivr", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Recupera la clave de acceso de un cliente                  */
/* Autor   : Danilo A. Leiva Espinoza                 Fecha: 05-02-2000 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecClave(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESO clave;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clave.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClaveDeAcceso(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_FECHA , clave.fechaCambioClave, 0);
   Fadd32(fml, CLI_CONTRASENA, clave.contrasena, 0);
   Fadd32(fml, CLI_USUARIO, (char *)&clave.ejecutivo, 0);
   Fadd32(fml, CLI_ESTADO_EMISION, (char *)&clave.estadoEmision, 0);
   Fadd32(fml, CLI_FECHA_EMISION, clave.fechaEmision, 0);
   Fadd32(fml, CLI_ESTADO, (char *)&clave.estado, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar la clave de acceso de un cliente                 */
/* Autor   : Danilo A. Leiva Espinoza                 Fecha: 05-02-2000 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliModClave(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLAVEDEACCESO clave;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&clave.rutCliente, 0);
   Fget32(fml, CLI_CONTRASENA, 0, clave.contrasena, 0);

   if (Foccur32(fml, CLI_USUARIO) > 0)
   {
      Fget32(fml, CLI_USUARIO, 0, (char *)&clave.ejecutivo, 0);
      clave.estado = CLI_CLAVE_INACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_POR_EMITIR;
   }
   else
   {
      clave.ejecutivo = 0;
      clave.estado = CLI_CLAVE_ACTIVA;
      clave.estadoEmision = CLI_CLAVE_EMISION_RECEPCIONADA;
   }

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarClaveDeAcceso(&clave);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no posee clave de acceso", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar la clave de acceso del cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Mantener Cliente                                           */
/* Autor   : Dinorah Painepan             Fecha:  Enero   2003          */
/************************************************************************/
void CliInsOcasional(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliOCASIONAL ocasional;
   short  tipo;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
   Fget32(fml, CLI_TIPO_MODIFICACION, 0, (char *)&tipo, 0);

  /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUTX, 0, (char *)&ocasional.rut, 0);
   if (tipo == CLI_MODIFICA)
   {
        sts = SqlCliRecuperarOcasional(&ocasional);
        if (sts != SQL_SUCCESS)
        {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente ocasional- ERROR: No se logro recuperar info. del cliente", 0);

         if (transaccionGlobal == 0)
                        tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }

   Fget32(fml, CLI_DVX, 0, ocasional.dv, 0);
   Fget32(fml, CLI_NOMBRES, 0, ocasional.nombres, 0);
   Fget32(fml, CLI_APELLIDO_PATERNO, 0, ocasional.apellidoPaterno, 0);
   Fget32(fml, CLI_APELLIDO_MATERNO, 0, ocasional.apellidoMaterno, 0);
   Fget32(fml, CLI_CALLE_NUMERO, 0, ocasional.calleNumero, 0);
   Fget32(fml, CLI_VILLA_POBLACION, 0, ocasional.villaPoblacion, 0);
   Fget32(fml, CLI_DEPARTAMENTO, 0, ocasional.departamento, 0);
   Fget32(fml, CLI_COMUNA, 0, (char *)&ocasional.comuna, 0);
   Fget32(fml, CLI_CIUDAD, 0, (char *)&ocasional.ciudad, 0);
   Fget32(fml, CLI_ACTIVIDAD, 0, (char *)&ocasional.actividad, 0);

   if (tipo != CLI_MODIFICA)
   {
        sts = SqlCliCrearOcasional(ocasional);
        if (sts != SQL_SUCCESS)
        {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear cliente ocasional - ERROR: No se pudo crear al cliente.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }
   }
   else
   {
        sts = SqlCliModificarOcasional(ocasional);
        if (sts != SQL_SUCCESS)
        {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar cliente ocasional - ERROR: No se pudo crear al cliente.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
        }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Obtener Cliente                                            */
/* Autor   : Dinorah Painepan             Fecha: Enero  2003            */
/************************************************************************/
void CliRecOcasional(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliOCASIONAL ocasional;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&ocasional.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT, 0L) == -1)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO,
            "Recuperar cliente - ERROR: Fallo al iniciar transaccion.", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }

     sts = SqlCliRecuperarOcasional(&ocasional);

   if (sts != SQL_SUCCESS)
   {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente no existe.", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la información del cliente.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_DVX, ocasional.dv, 0);
   Fadd32(fml, CLI_NOMBRES, ocasional.nombres, 0);
   Fadd32(fml, CLI_APELLIDO_PATERNO, ocasional.apellidoPaterno, 0);
   Fadd32(fml, CLI_APELLIDO_MATERNO, ocasional.apellidoMaterno, 0);
   Fadd32(fml, CLI_CALLE_NUMERO, ocasional.calleNumero, 0);
   Fadd32(fml, CLI_VILLA_POBLACION, ocasional.villaPoblacion, 0);
   Fadd32(fml, CLI_DEPARTAMENTO, ocasional.departamento, 0);
   Fadd32(fml, CLI_CIUDAD, (char *)&ocasional.ciudad, 0);
   Fadd32(fml, CLI_COMUNA, (char *)&ocasional.comuna, 0);
   Fadd32(fml, CLI_ACTIVIDAD, (char *)&ocasional.actividad, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Modificar direccion por solicitud del cliente              */
/* Autor   :                              Fecha: Julio   1998           */
/* Modifica:                              Fecha: Abril   2002           */
/************************************************************************/

void CliModDatPerSol(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   char cadena[201];
   long sts;
   long correlativo;
   tCliDIRECCION direccion;
   tCliDIRECCION direccionAnterior;
   tCliREGMOD regModificacion;
   tTcrSOLICITUDCLIENTE solicitudCliente;
   tCliTELEFONO telefono;
   tCliTELEFONO telefonoAnterior;
   tCliCLIENTE cliente;
   short inicioDireccion=0;
   short i;
   long  horaLng;
   char  fechaProcesoHora[14+1];

   /******   Buffer de entrada   ******/

   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CORRELATIVO , 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
    if (tpbegin(TIME_OUT, 0L) == -1)
    {
      Fadd(fml, HRM_MENSAJE_SERVICIO,
        "Error al iniciar Transaccion Modificar Dirección.", 0);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
    }

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********* ContinuidadHoraria [FIN] *********/

    solicitudCliente.correlativo = correlativo;


    sts=RecuperarNuevaDireccion(&solicitudCliente);

    if ( sts != SRV_SUCCESS)
    {
        Fadd(fml, HRM_MENSAJE_SERVICIO, "Error recuperar solicitud cliente.", 0);
        if (transaccionGlobal == 0)
          tpabort(0L);

        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }

    strcpy(fechaProcesoHora,gFechaProceso);
    horaLng = f_horaComoLong();
    f_strcatlng(fechaProcesoHora, horaLng, 6);


    for (i=0; i<3;i++)
    {

       memset((void *)&direccion,0,sizeof(tCliDIRECCION));
       memset((void *)&telefono,0,sizeof(tCliTELEFONO));


      direccion.rutCliente           = solicitudCliente.rutCliente;
      telefono.rutCliente            = solicitudCliente.rutCliente;
      direccionAnterior.rutCliente   = solicitudCliente.rutCliente;
      telefonoAnterior.rutCliente    = solicitudCliente.rutCliente;


       sts= DesconcatenaDireccion(solicitudCliente,i,&inicioDireccion,&direccion,&telefono);
        
       if( sts != SRV_SUCCESS ) continue;

       direccionAnterior.tipoDireccion = direccion.tipoDireccion;

       sts = SqlCliRecuperarUnaDireccion(&direccionAnterior);
       if (sts != SQL_SUCCESS)
       {
          if ( sts == SQL_NOT_FOUND )
          {
              sts= SqlCliCrearDireccion(direccion);
              if( sts != SQL_SUCCESS)
              {
                  Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al insertar dirección.", 0);
                  if (transaccionGlobal == 0) tpabort(0L);
                  tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
              }

              strcpy(cadena," ");
              LlenarRegistroModificacion(solicitudCliente,fechaProcesoHora,cadena,&regModificacion);
          }
          else
          {
              Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar dirección.", 0);
              if (transaccionGlobal == 0) tpabort(0L);
              tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
          }
       }
       else
       {
            sts = SqlCliModificarDireccionSolCli(direccion);

            if (sts != SQL_SUCCESS)
            {
                Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);
                if (transaccionGlobal == 0) tpabort(0L);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
            }

           /*grabar Reg. Modif.*/

            ConcatenaDireccion(direccionAnterior, cadena);
            LlenarRegistroModificacion(solicitudCliente,fechaProcesoHora,cadena,&regModificacion);

       }

        if (solicitudCliente.nuevaDireccion == CLI_DIRECCION_PARTICULAR)
                 regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_PARTICULAR;
        else
        {
              if (solicitudCliente.nuevaDireccion == CLI_DIRECCION_COMERCIAL)
                  regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_COMERCIAL;
              else
                  regModificacion.tipoModificacion = CLI_MODIFICACION_DIRECCION_ENVIO_ALTERNATIVA;
        }

        sts = SqlCliCrearRegMod(regModificacion);
        if (sts != SQL_SUCCESS)
        {
             if (sts == SQL_DUPLICATE_KEY)
               Fadd(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
             else
                Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

             if (transaccionGlobal == 0) tpabort(0L);

             tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        if (strcmp(telefono.numero,"") == 0) continue;

        telefonoAnterior.tipoTelefono  = telefono.tipoTelefono;

        sts = SqlCliRecuperarUnTelefono(&telefonoAnterior);
        if (sts != SQL_SUCCESS)
        {
          if ( sts == SQL_NOT_FOUND )
          {
              sts= SqlCliCrearTelefono(telefono);
              if( sts != SQL_SUCCESS)
              {
                  Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al insertar teléfono.", 0);
                  if (transaccionGlobal == 0) tpabort(0L);
                  tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
              }

           }
           else
           {
              Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar teléfono.", 0);
              if (transaccionGlobal == 0) tpabort(0L);
              tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
           }
         }
         else
         {
            sts = SqlCliModificarTelefono(telefono);

            if (sts != SQL_SUCCESS)
             {
                Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al modificar teléfono", 0);
                if (transaccionGlobal == 0) tpabort(0L);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
            }
        }
     }

    sts = ModificarModoProcesoSolicitudCliente(correlativo);
    if (sts  != SRV_SUCCESS)
    {
        Fadd(fml, HRM_MENSAJE_SERVICIO, "Error al modificar modo de proceso de la solicitud", 0);
        if (transaccionGlobal == 0) tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    } 


    if (transaccionGlobal == 0) tpcommit(0L);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recuperar documentos firmados                               */
/* Autor   : Andrea Sanchez A.                        Fecha: 01-10-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecTodDocFir(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliDOCUMENTACIONFIRMADAAUX  *documento;
   int totalRegistros=0;
   long         rutCliente;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUT,         0, (char *)&rutCliente, 0);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarTodosLosDocumentosFirmados(rutCliente);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar documentación firmada", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarDocumentosNoFirmados(lista, rutCliente);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar segmentos", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliDOCUMENTACIONFIRMADAAUX)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      documento = (tCliDOCUMENTACIONFIRMADAAUX  *) QGetItem( lista, i);

      Fadd32(fml, CLI_NOMBRE,           documento->descripcionTipoDocumento , i);
      Fadd32(fml, CLI_TIPO_DOCUMENTO,     (char *)&documento->tipoDocumento , i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar solicitud telefonica                             */
/* Autor   : Andrea Sanchez Arriagada              Fecha: 19-01-2004    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliRecSolTelef(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long i=0, sts=0, ciclo=0;
   short secuencia=0;
   tCliSolicitudTelefonica solTelefonica;

   /******   Buffer de entrada   ******/

   memset((void *)&solTelefonica,0,sizeof(tCliSolicitudTelefonica));

   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_SUCURSAL, 0, (char *)&solTelefonica.sucursal, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&solTelefonica.ejecutivo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Recuperar solicitud telefonica", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********* ContinuidadHoraria [FIN] *********/


      sts = SqlCliRecuperarSecuenciaSucursal(solTelefonica.sucursal, &secuencia);

      switch (secuencia)
      {
        case 1:
          ciclo = 0;
          break;
        case 2:
          ciclo = 10;
          break;
        case 3:
          ciclo = 20;
          break;
        case 4:
          ciclo = 30;
          break;
        case 5:
          ciclo = 40;
          break;
        case 6:
          ciclo = 50;
          break;
        case 7:
          ciclo = 60;
          break;
        case 8:
          ciclo = 70;
          break;
        case 9:
          ciclo = 80;
          break;
        case 10:
          ciclo = 90;
          break;
      }

      for (i=0; i<ciclo; i++);

      sts = SqlCliRecuperarSolicitudCliente(&solTelefonica, gFechaProceso);

      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe registro de solicitud telefonica.", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar solicitud telefonica.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      Fadd32(fml, CLI_CODIGO_SISTEMA,    (char *)&solTelefonica.codigoSistema, 0);
      Fadd32(fml, CLI_SOLICITUD,    (char *)&solTelefonica.numeroSolicitud, 0);
      Fadd32(fml, CLI_FECHA,    solTelefonica.fechaCreacion, 0);
      Fadd32(fml, CLI_CORRELATIVO,    (char *)&solTelefonica.correlativo, 0);
      Fadd32(fml, CLI_RUT,    (char *)&solTelefonica.rutCliente, 0);
      Fadd32(fml, CLI_SUCURSAL,    (char *)&solTelefonica.sucursal, 0);
      Fadd32(fml, CLI_USUARIO,    (char *)&solTelefonica.ejecutivo, 0);
      Fadd32(fml, CLI_FECHA_EXPIRACION,    solTelefonica.fechaExpiracion, 0);
      Fadd32(fml, CLI_PRODUCTO,    (char *)&solTelefonica.producto, 0);
      Fadd32(fml, CLI_RTA_MONTO,    (char *)&solTelefonica.montoSolicitud, 0);
      Fadd32(fml, CLI_PROMOCION,    (char *)&solTelefonica.promocion, 0);
      Fadd32(fml, CLI_ESTADO,    (char *)&solTelefonica.estado, 0);
      Fadd32(fml, CLI_CODIGO,    (char *)&solTelefonica.numeroGestiones, 0);
      Fadd32(fml, CLI_SISTEMA,    solTelefonica.descripcionSistema, 0);
      Fadd32(fml, CLI_DESCRIPCION_PROD,    solTelefonica.descripcionProducto, 0);
      Fadd32(fml, CLI_DESCRIPCION_PROM,    solTelefonica.descripcionPromocion, 0);
      Fadd32(fml, CLI_FECHA_CANCELACION,    solTelefonica.fechaTerminoSuspension, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar estados solicitud telefonica                     */
/* Autor   : Andrea Sanchez Arriagada              Fecha: 21-01-2004    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliRecEstSolTel(TPSVCINFO *rqst)
{
   FBFR32       *fml;
   int          transaccionGlobal;
   long         sts, i;
   Q_HANDLE     *lista;
   tCliEstadoSolicitudTelefonica *estado;
   long         totalRegistros;

   lista = (Q_HANDLE *)QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarEstadosSolicitudTelefonica(lista);
   if (sts != SQL_SUCCESS)
   {
     if ( sts == SQL_NOT_FOUND )
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen estados definidos para solicitud telefonica", 0);
     else
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar estados de solicitud telefonica", 0);
    TRX_ABORT(transaccionGlobal);
    tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliEstadoSolicitudTelefonica)* totalRegistros));

   for (i = 0; i < totalRegistros ; i++)
   {
    estado = (tCliEstadoSolicitudTelefonica *)QGetItem(lista,i);

    Fadd32(fml, CLI_CODIGO , (char *)&estado->codigo, 0);
    Fadd32(fml, CLI_DESCRIPCION, estado->descripcion, 0);
   }

   QDelete(lista);

   if (transaccionGlobal == 0)
         tpcommit(0L);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar estado de solicitud telefonica                   */
/* Autor   : Andrea Sanchez Arriagada              Fecha: 21-01-2004    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliModEstSolTel(TPSVCINFO *rqst)
{

   FBFR32 *fml;
   int transaccionGlobal;
   tCliSolicitudTelefonica solTelefonica;
   long sts;
   short  dbEstado = CLI_ESTADO_SUSPENDIDA;
   tCliPARAMETRO parametro;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   memset((void *)&solTelefonica,0,sizeof(tCliSolicitudTelefonica));

   Fget32(fml, CLI_ESTADO, 0, (char *)&solTelefonica.estado, 0);
   Fget32(fml, CLI_CODIGO_SISTEMA, 0, (char *)&solTelefonica.codigoSistema, 0);
   Fget32(fml, CLI_SOLICITUD, 0, (char *)&solTelefonica.numeroSolicitud, 0);
   Fget32(fml, CLI_FECHA, 0, solTelefonica.fechaCreacion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********** ContinuidadHoraria [INI] **********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********** ContinuidadHoraria [FIN] **********/


   if (solTelefonica.estado == dbEstado)
   {
      sts = SqlCliRecuperarParametro(&parametro);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar registro", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }


      ProximaFecha(gFechaProceso, parametro.diasSuspension, solTelefonica.fechaTerminoSuspension);
      solTelefonica.fechaTerminoSuspension[8] ='\0';
   }
   else
      solTelefonica.fechaTerminoSuspension[0] = '\0';
/*      strcpy(solTelefonica.fechaTerminoSuspension,'');*/

   sts = SqlCliModificarEstadoSolicitudTelefonica(&solTelefonica);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliCrearMovimientoSolTelefonica(solTelefonica, gFechaProceso);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"No se pudo insertar movimiento de solicitud telefonica.",0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recuperar Clientes por nombre                               */
/* Autor   :                                          Fecha:             */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecCliPorNom(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliCLIENTE           cliente;
   tCliCLIENTE           *datosCliente;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_NOMBRE,         0, cliente.nombres, 0);
   Fget32(fml, CLI_APELLIDOMATERNO,0, cliente.apellidoMaterno, 0);
   Fget32(fml, CLI_APELLIDOPATERNO,0, cliente.apellidoPaterno, 0);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Recuperar por Nombre Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarClientePorNombre(lista, cliente);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar nombres", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliCLIENTE)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      datosCliente = (tCliCLIENTE  *) QGetItem( lista, i);

      Fadd32(fml, CLI_NOMBRE,           datosCliente->nombres, i);
      Fadd32(fml, CLI_APELLIDOMATERNO,  datosCliente->apellidoMaterno, i);
      Fadd32(fml, CLI_APELLIDOPATERNO,  datosCliente->apellidoPaterno, i);
      Fadd32(fml, CLI_RUT,              (char *)&datosCliente->rut, i);
      Fadd32(fml, CLI_DVX,              datosCliente->dv, i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recuperar Clientes Empresa por razonSocial                  */
/* Autor   :                                          Fecha:             */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecEmpPorNom(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliCLIENTEEMPRESA    clienteEmp;
   tCliCLIENTEEMPRESA    *datosClienteEmp;

   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RAZON_SOCIAL,         0, clienteEmp.razonSocial, 0);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente Emp - Recuperar por Nombre Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarClienteEmpresaPorNombre(lista, clienteEmp);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente Emp - ERROR: No se logro recuperar nombres", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliCLIENTE)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      datosClienteEmp = (tCliCLIENTEEMPRESA  *) QGetItem( lista, i);

      Fadd32(fml, CLI_RAZON_SOCIAL,     datosClienteEmp->razonSocial, i);
      Fadd32(fml, CLI_RUT,              (char *)&datosClienteEmp->rut, i);
      Fadd32(fml, CLI_DVX,              datosClienteEmp->dv, i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************/
/* Objetivo: Recuperar todos los tipos de sociedades de empresa definidos */
/* Autor   : Boris Contreras Mac-lean                   Fecha: 28-10-2003 */
/* Modifica:                                            Fecha:            */
/**************************************************************************/
void CliRecTodTipSoc(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTIPOSOCIEDADEMPRESA *tipoSociedad;
   Q_HANDLE *lista;
   int totalRegistros=0, i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(20,20)) == NULL)
   {
      sts = SQL_MEMORY;
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarTipoDeSociedadTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen tipos de sociedades definidos.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar los tipos de sociedades definidos.", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;

   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3*totalRegistros,
             sizeof(tCliTIPOSOCIEDADEMPRESA)* totalRegistros));

   for (i=0; i < lista->count ; i++)
   {
      tipoSociedad = (tCliTIPOSOCIEDADEMPRESA *)QGetItem(lista,i);

      Fadd32(fml, CLI_CODIGO, (char *)&tipoSociedad->codigo, 0);
      Fadd32(fml, CLI_DESCRIPCION, tipoSociedad->descripcion, 0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/*******************************************************************/
/* Objetivo: Recuperar un tipo de sociedad definido para empresa   */
/* Autor   : Boris Contreras Mac-lean            Fecha: 28-10-2003 */
/* Modifica:                                     Fecha:            */
/*******************************************************************/
void CliRecTipSocEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTIPOSOCIEDADEMPRESA tipoSociedad;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_CODIGO,     0, (char *)&tipoSociedad.codigo, 0);


   sts = SqlCliRecuperarTipoDeSociedadEmpresa(&tipoSociedad);
   if (sts != SQL_SUCCESS)
      {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el tipo de sociedad ingresado.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar un tipo de sociedad solicitado.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }


   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3, sizeof(tCliTIPOSOCIEDADEMPRESA)));

   Fadd32(fml, CLI_CODIGO, (char *)&tipoSociedad.codigo, 0);
   Fadd32(fml, CLI_DESCRIPCION, tipoSociedad.descripcion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Crear un nuevo tipo de sociedad para empresa               */
/* Autor   : Boris Contreras Mac-lean     Fecha: 28/10/2003             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsTipSocEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliTIPOSOCIEDADEMPRESA tipoSociedad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO,      0, (char *)&tipoSociedad.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,          tipoSociedad.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear tipo de sociedad empresa.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearTipoSociedadEmpresa(tipoSociedad);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El tipo de sociedad de empresa ya existe.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear un tipo de sociedad empresa.", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}


/************************************************************************/
/* Objetivo: Modificar un tipo de sociedad para empresa                 */
/* Autor   : Boris Contreras Mac-lean     Fecha: 28/05/2003             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModTipSocEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliTIPOSOCIEDADEMPRESA tipoSociedad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO     , 0, (char *)&tipoSociedad.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,          tipoSociedad.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar tipo de sociedad empresa.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarTipoSociedadEmpresa(tipoSociedad);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar tipo de sociedad empresa", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Eliminar un tipo de sociedad para empresa                  */
/* Autor   : Boris Contreras mac-lean     Fecha: 28/10/2003             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmTipSocEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliTIPOSOCIEDADEMPRESA tipoSociedad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO, 0, (char *)&tipoSociedad.codigo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar tipo de sociedad empresa.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliEliminarTipoSociedadEmpresa(tipoSociedad);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar tipo de sociedad empresa", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}



/*************************** Segmentacion de Clientes ***********************/
/************************************************************************/
/* Objetivo: Recuperar el segmento del cliente                          */
/* Autor   :                                       Fecha: 13-05-2003    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliInsSegCli(TPSVCINFO *rqst)
{

   FBFR32 *fml;
   int transaccionGlobal;
   tCliSEGMENTOCLIENTE segmentoCliente;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&segmentoCliente.rutCliente, 0);
   Fget32(fml, CLI_SEGMENTO, 0, (char *)&segmentoCliente.segmento, 0);
   Fget32(fml, CLI_FECHA, 0, segmentoCliente.fechaSegmentacion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliInsertarsegmentoCliente(&segmentoCliente);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El codigo ya existe", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/************************************************************************/
/* Objetivo: Recuperar el segmento del cliente                          */
/* Autor   :                                       Fecha: 13-05-2003    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliModSegCli(TPSVCINFO *rqst)
{

   FBFR32 *fml;
   int transaccionGlobal;
   tCliSEGMENTOCLIENTE segmentoCliente;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&segmentoCliente.rutCliente, 0);
   Fget32(fml, CLI_SEGMENTO, 0, (char *)&segmentoCliente.segmento, 0);
   Fget32(fml, CLI_FECHA, 0, segmentoCliente.fechaSegmentacion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarsegmentoCliente(&segmentoCliente);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar registro", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Recuperar el segmento del cliente                          */
/* Autor   :                                       Fecha: 13-05-2003    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliRecSegCli(TPSVCINFO *rqst)
{

   FBFR32 *fml;
   int transaccionGlobal;
   int i;
   tCliSEGMENTOCLIENTE *segmentoCliente;
   Q_HANDLE          *lstSegmentoCliente;
   long sts;
   long rutCliente;

   lstSegmentoCliente = (Q_HANDLE *) QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ( Foccur32(fml, CLI_FECHA)  == 0 )
   {
      sts = SqlCliRecuperarsegmentoClienteTodos(lstSegmentoCliente, rutCliente);

      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar los segmentos cliente" , 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      for (i=0;i<lstSegmentoCliente->count;i++)
      {
         segmentoCliente = (tCliSEGMENTOCLIENTE *)QGetItem(lstSegmentoCliente,i);
         Fadd32(fml, CLI_SEGMENTO,    (char *)&segmentoCliente->segmento, 0);
         Fadd32(fml, CLI_FECHA,     segmentoCliente->fechaSegmentacion,     0);
         Fadd32(fml, CLI_RUTX,      (char *)&segmentoCliente->rutCliente, 0);
      }
      QDelete(lstSegmentoCliente);
   }
   else
   {
      segmentoCliente = malloc(sizeof(tCliSEGMENTOCLIENTE));

      segmentoCliente->rutCliente = rutCliente;

      Fget32(fml, CLI_FECHA, 0, (char *)&segmentoCliente->fechaSegmentacion, 0);

      sts = SqlCliRecuperarsegmentoCliente(segmentoCliente);

      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar segmento cliente" , 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      Fadd32(fml, CLI_SEGMENTO,    (char *)&segmentoCliente->segmento, 0);
      Fadd32(fml, CLI_CODIGO,      (char *)&segmentoCliente->segmento, 0);
      Fadd32(fml, CLI_FECHA,     segmentoCliente->fechaSegmentacion,     0);
      Fadd32(fml, CLI_RUTX,      (char *)&segmentoCliente->rutCliente, 0);
      free(segmentoCliente);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar el segmento del cliente                          */
/* Autor   :                                       Fecha: 13-05-2003    */
/* Modifica:                                       Fecha:               */
/************************************************************************/
void CliRecSegVar(TPSVCINFO *rqst)
{

   FBFR32 *fml;
   int transaccionGlobal;
   int i;
   tCliSEGMENTOVARIABLE *segmentoVariable;
   Q_HANDLE          *lstSegmentoVariable;
   long sts;

   lstSegmentoVariable = (Q_HANDLE *) QNew();

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ( Foccur32(fml, CLI_VARIABLESEGMENTO) == 0 )
   {
      sts = SqlCliRecuperarSegmentoVariableTodos(lstSegmentoVariable);

      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar los segmento variable" , 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      for (i=0;i<lstSegmentoVariable->count;i++)
      {
         segmentoVariable = (tCliSEGMENTOVARIABLE *)QGetItem(lstSegmentoVariable,i);
         Fadd32(fml, CLI_VARIABLESEGMENTO,    (char *)&segmentoVariable->codigo, 0);
         Fadd32(fml, CLI_NOMBRE,              segmentoVariable->nombre,     0);
         Fadd32(fml, CLI_TIPOVALOR,           (char *)&segmentoVariable->tipoValor, 0);
      }
      QDelete(lstSegmentoVariable);
   }
   else
   {
      segmentoVariable = malloc(sizeof(tCliSEGMENTOVARIABLE));

      Fget32(fml, CLI_VARIABLESEGMENTO, 0, (char *)&segmentoVariable->codigo, 0);

      sts = SqlCliRecuperarSegmentoVariable(segmentoVariable);

      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar segmento variable" , 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      Fadd32(fml, CLI_VARIABLESEGMENTO,    (char *)&segmentoVariable->codigo, 0);
      Fadd32(fml, CLI_NOMBRE,              segmentoVariable->nombre,     0);
      Fadd32(fml, CLI_TIPOVALOR,           (char *)&segmentoVariable->tipoValor, 0);
      free(segmentoVariable);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/*************************************************************************/
/* Objetivo: Recuperar segmentos                                         */
/* Autor   : Elizabeth Mery                           Fecha: 15-01-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecLisSeg(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliSEGMENTO          *segmento;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarListaDeSegmentos(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No hay segmentos", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar segmentos", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliSEGMENTO)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      segmento = (tCliSEGMENTO  *) QGetItem( lista, i);

      Fadd32(fml, CLI_CODIGO,  (char *)&segmento->codigo, i);
      Fadd32(fml, CLI_NOMBRE,           segmento->nombre , i);
      Fadd32(fml, CLI_PRIORIDAD, (char*)&segmento->prioridad,i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/*************************************************************************/
/* Objetivo: Insertar segmento                                           */
/* Autor   : Elizabeth Mery                           Fecha: 15-01-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliInsSeg(TPSVCINFO *rqst)
{
   FBFR32              *fml;
   int                 transaccionGlobal;
   long                sts;
   tCliSEGMENTO        segmento;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_CODIGO, 0, (char *)&segmento.codigo, 0);
   Fget32(fml, CLI_NOMBRE, 0, segmento.nombre    , 0);
   Fget32(fml, CLI_PRIORIDAD ,0 , (char *)&segmento.prioridad,0);

   sts = SqlCliInsertarSegmento(&segmento);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Crear segmento  - ERROR: No se pudo crear el segmento.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Eliminar segmento                                           */
/* Autor   : Elizabeth Mery                           Fecha: 15-01-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliElmSeg (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliSEGMENTO segmento;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO, 0, (char *)&segmento.codigo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliEliminarSegmento(&segmento);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Segmento no existe" , 0);
      else if (sts == SQL_REFERENCIAL_INTEGRITY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Segmento no puede ser eliminado por integridad de datos", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO,"Eliminar segmento - ERROR: No se pudo eliminar el segmento.", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Actualizar segmentos                                        */
/* Autor   : Elizabeth Mery                           Fecha: 15-01-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliActSeg (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliSEGMENTO segmento;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO, 0, (char *)&segmento.codigo, 0);
   Fget32(fml, CLI_NOMBRE, 0, segmento.nombre    , 0);
   Fget32(fml, CLI_PRIORIDAD,0, (char *)&segmento.prioridad,0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarSegmento(&segmento);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Actualizar segmento - ERROR: No se pudo actualizar el segmento.", 0);
      userlog("%s: Error en funcion SqlCctModificarSegmentoCliente.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/*************************************************************************/
/* Objetivo: Ingresar variables segmentos                                */
/* Autor   : Danilo Leiva E.                          Fecha: 08-07-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliInsSegVal(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliSEGMENTOVALOR segmentoValor;
   long sts;
   long ocurrencia=0;
   int i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_SEGMENTO,         0, (char *)&segmentoValor.segmento, 0);
   Fget32(fml, CLI_VARIABLESEGMENTO, 0, (char *)&segmentoValor.variableSegmento, 0);
   Fget32(fml, CLI_USUARIO,          0, (char *)&segmentoValor.ejecutivo, 0);
   Fget32(fml, CLI_SUCURSAL,         0, (char *)&segmentoValor.sucursal, 0);

   if (Foccur32(fml, CLI_VALORHASTA) > 0)
      Fget32(fml, CLI_VALORHASTA, 0, (char *)&segmentoValor.valorHasta, 0);
   else
      segmentoValor.valorHasta = 0;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarSegmentoValorTermino(&segmentoValor);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo actualizar el segmento.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   ocurrencia = Foccur32(fml, CLI_VALOR);

   for (i=0; i < ocurrencia; i++)
  {
      Fget32(fml, CLI_VALOR, i, (char *)&segmentoValor.valor, 0);

      sts = SqlCliInsertarSegmentoValor(&segmentoValor);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "El codigo ya existe", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar registro", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recuperar valores de variables de segmentos                 */
/* Autor   : Danilo A. Leiva E.                       Fecha: 10-07-2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecLisSegVal(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliSEGMENTOVALOR     *segmentoValor;
   short                 segmento;
   short                 variableSegmento;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_SEGMENTO,         0, (char *)&segmento, 0);
   Fget32(fml, CLI_VARIABLESEGMENTO, 0, (char *)&variableSegmento, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarSegmentoValorTodos(segmento, variableSegmento, lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen valores para esta variable en el segmento", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar valores para esta variable en el segmento", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliSEGMENTOVALOR)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      segmentoValor = (tCliSEGMENTOVALOR  *) QGetItem( lista, i);

      Fadd32(fml, CLI_VALOR,      (char *)&segmentoValor->valor, i);
      Fadd32(fml, CLI_VALORHASTA, (char *)&segmentoValor->valorHasta , i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Carrera                                              */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsCarrera(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliCARRERA carrera;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CARRERA, 0, (char *)&carrera.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,     carrera.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Carrera.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearCarrera(carrera);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La carrera ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear carrera", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Modificar Carrera                                          */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModCarrera(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliCARRERA carrera;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CARRERA, 0, (char *)&carrera.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,     carrera.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar Carrera.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarCarrera(carrera);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar carrera", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Eliminar Carrera                                          */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmCarrera(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliCARRERA carrera;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CARRERA, 0, (char *)&carrera.codigo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar Carrera.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliEliminarCarrera(carrera);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar carrera", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Crear Universidad                                          */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsUniversi(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliUNIVERSIDAD universidad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_UNIVERSIDAD, 0, (char *)&universidad.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,     universidad.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear universidad.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliCrearUniversidad(universidad);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La universidad ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear universidad", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}


/************************************************************************/
/* Objetivo: Modificar universidad                                          */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModUniversi(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliUNIVERSIDAD universidad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_UNIVERSIDAD, 0, (char *)&universidad.codigo, 0);
   Fget32(fml, CLI_DESCRIPCION, 0,     universidad.descripcion, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Modificar universidad.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliModificarUniversidad(universidad);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar universidad", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Eliminar universidad                                          */
/* Autor   : Elena Lopez  .               Fecha:  Agosto   2003         */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmUniversi(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliUNIVERSIDAD universidad;
   int transaccionGlobal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_UNIVERSIDAD, 0, (char *)&universidad.codigo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Eliminar universidad.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }

   sts = SqlCliEliminarUniversidad(universidad);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar universidad", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);

      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}


/*************************************************************************/
/* Objetivo: Recuperar segmento actual de cliente                        */
/* Autor   : Erica Moreira V.                         Fecha: Marzo 2003  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/

void CliRecMaxSegCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliSEGMENTOCLIENTE  segmentoCliente;
   tCliCLIENTE cliente;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   Fget32(fml, CLI_RUTX, 0, (char *)&segmentoCliente.rutCliente, 0);
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarMaximoSegmentoCliente(&segmentoCliente);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar segmento cliente" , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }


   Fadd32(fml, CLI_SEGMENTO,    (char *)&segmentoCliente.segmento, 0);
   Fadd32(fml, CLI_NOMBRE,      segmentoCliente.nombre , 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*********************************************************************************************************************/
/********************************************* INICIO MICROEMPRESARIO ************************************************/


/************************************************************************/
/* Objetivo: Recupera lista de rubro                                    */
/* Autor   : Fabiola Urqueta P.           Fecha: 09-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRubrMEMPT(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   Q_HANDLE   *lista;
   tCliMEMP_RUBRO  *MEMP_Rubro;
   tCliMEMP_RUBROVETADO  MEMP_RubroVetado;

   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_RubroTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de rubro." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)2, (short)0, (short)2, (long)150);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      MEMP_Rubro = (tCliMEMP_RUBRO *) QGetItem( lista, i);

      Fadd32( fml, CLI_RUBRO,                (char *)&MEMP_Rubro->codigo,    0);
      Fadd32( fml, CLI_DESCRIPCION,          MEMP_Rubro->descripcion,        0);
      Fadd32( fml, CLI_RUBRO_SBIF,           (char *)&MEMP_Rubro->rubroSBIF, 0);

      MEMP_RubroVetado.rubro = MEMP_Rubro->codigo;

      sts = SqlCliRecuperarMEMP_RubroVetado(MEMP_RubroVetado);
      if (sts == SQL_CRITICAL_ERROR)
      {
         QDelete(lista);
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar el detalle de un rubro." , 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      if (sts == SQL_SUCCESS)
         Fadd32( fml, CLI_ES_RUBRO_VETADO, CLI_SI, 0);
      else
         Fadd32( fml, CLI_ES_RUBRO_VETADO, CLI_NO, 0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta un rubro de microempresario                        */
/* Autor   : Boris Contreras M.           Fecha: 10-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsRubroMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBRO mEMP_Rubro;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUBRO,        0,(char *)&mEMP_Rubro.codigo, 0);
   sts |= Fget32(fml, CLI_DESCRIPCION,  0,mEMP_Rubro.descripcion, 0);
   sts |= Fget32(fml, CLI_RUBRO_SBIF,   0,(char *)&mEMP_Rubro.rubroSBIF, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarMEMP_Rubro(&mEMP_Rubro);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta el nuevo rubro de microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertarMEMP_Rubro.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Elimina rubro microEmpresario                              */
/* Autor   : Boris Contreras Mac-lean     Fecha: 10-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmRubroMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBRO mEMP_Rubro;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_RUBRO, 0, (char *)&mEMP_Rubro.codigo, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminarMEMP_Rubro(&mEMP_Rubro);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar el rubro del microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera uno de rubro SBIF                                 */
/* Autor   : Boris Contreras Mac-lean     Fecha: 12-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRubroSBIF(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   tCliMEMP_RUBROSBIF  MEMP_RubroSBIF;

   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32( fml, CLI_RUBRO_SBIF, 0,        (char *)&MEMP_RubroSBIF.codigo,    0);
   if (sts == -1)
   {
      userlog("%s: Error al ingreso de los FML de entrada.", rqst->name);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar un de rubro SBIF." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }


   sts = SqlCliRecuperarMEMP_RubroSBIF(&MEMP_RubroSBIF);
   if (sts != SQL_SUCCESS)
   {
      userlog("%s: Error al recuperar datos de SqlCliRecuperarMEMP_RubroSBIF.", rqst->name);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar un rubro SBIF." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)1, (short)1, (short)0, (short)1, (long)150);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }


   Fadd32( fml, CLI_RUBRO_SBIF,           (char *)&MEMP_RubroSBIF.codigo,    0);
   Fadd32( fml, CLI_DESCRIPCION,          MEMP_RubroSBIF.descripcion,         0);


   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista de rubro SBIF                               */
/* Autor   : Boris Contreras Mac-lean     Fecha: 10-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRubroSBIT(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   Q_HANDLE   *lista;
   tCliMEMP_RUBROSBIF  *MEMP_RubroSBIF;

   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_RubroSBIFTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de rubro SBIF." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)150);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      MEMP_RubroSBIF = (tCliMEMP_RUBROSBIF *) QGetItem(lista, i);

      Fadd32( fml, CLI_RUBRO_SBIF,           (char *) &MEMP_RubroSBIF->codigo,    0);
      Fadd32( fml, CLI_DESCRIPCION,          MEMP_RubroSBIF->descripcion,         0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta un rubro SBIF                                      */
/* Autor   : Boris Contreras M.           Fecha: 10-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsRubroSBIF(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBROSBIF mEMP_RubroSBIF;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUBRO_SBIF,   0,(char *)&mEMP_RubroSBIF.codigo, 0);
   sts |= Fget32(fml, CLI_DESCRIPCION,  0,mEMP_RubroSBIF.descripcion, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarMEMP_RubroSBIF(&mEMP_RubroSBIF);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta el nuevo rubro de SBIF." , 0);
     userlog("%s: Error en funcion SqlCliInsertarMEMP_RubroSBIF.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Elimina rubro SBIF                                         */
/* Autor   : Boris Contreras Mac-lean     Fecha: 10-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmRubroSBIF(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBROSBIF mEMP_RubroSBIF;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_RUBRO_SBIF, 0, (char *)&mEMP_RubroSBIF.codigo, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminarMEMP_RubroSBIF(&mEMP_RubroSBIF);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar el rubro del SBIF." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta un rubro vetado  de microempresario                */
/* Autor   : Fabiola Urqueta P.           Fecha: 21-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsRubVetMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBROVETADO mEMP_RubroVetado;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUBRO,    0,(char *)&mEMP_RubroVetado.rubro, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarMEMP_RubroVetado(&mEMP_RubroVetado);
   if(sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta rubro vetado microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertarMEMP_RubroVetado.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Elimina rubro vetado microEmpresario                       */
/* Autor   : Fabiola Urqueta P.           Fecha: 21-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmRubVetMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RUBROVETADO mEMP_RubroVetado;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_RUBRO, 0, (char *)&mEMP_RubroVetado.rubro, 0);
   if (sts == -1)
     {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminarMEMP_RubroVetado(&mEMP_RubroVetado);
   if(sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar rubro vetado microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista rubro vetado microEmpresario                */
/* Autor   : Fabiola Urqueta P.           Fecha: 21-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRubVetMET(TPSVCINFO *rqst)
{
   FBFR32                         *fml;
   int                            transaccionGlobal;
   Q_HANDLE                       *lista;
   tCliMEMP_RUBROVETADO           *mEMP_RubroVetado;

   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts =SqlCliRecuperarMEMP_RubroVetadoTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de rubro vetado microEmpresario." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      mEMP_RubroVetado = (tCliMEMP_RUBROVETADO *) QGetItem( lista, i);
      Fadd32(fml, CLI_RUBRO     ,   (char *)&mEMP_RubroVetado->rubro, 0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Insertar RDocumento Actividad MicroEmpresario              */
/* Autor   : Fabiola Urqueta P.           Fecha: 21-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDocActMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RDOCUMENTOACTIVIDAD mEMP_RDocumentoActividad;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_DOCUMENTO,             0, (char *)&mEMP_RDocumentoActividad.documento, 0);
   sts |= Fget32(fml, CLI_CLASIFICACIONACTIVIDAD, 0, (char *)&mEMP_RDocumentoActividad.clasificacionActividad, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarMEMP_RDocumentoActividad(&mEMP_RDocumentoActividad);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta rDocumento actividad microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertarMEMP_RDocumentoActividad.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/* Objetivo: Elimina RDocumento Actividad microempresario               */
/* Autor   : Fabiola Urqueta P.           Fecha: 21-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmDocActMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_RDOCUMENTOACTIVIDAD mEMP_RDocumentoActividad;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts =  Fget32(fml, CLI_DOCUMENTO, 0, (char *)&mEMP_RDocumentoActividad.documento, 0);
   sts |= Fget32(fml, CLI_CLASIFICACIONACTIVIDAD, 0, (char *)&mEMP_RDocumentoActividad.clasificacionActividad, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminarMEMP_RDocumentoActividad(&mEMP_RDocumentoActividad);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar rDocumento actividad microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***************************************************************************/
/* Objetivo: Recupera lista Clasificacion de activiadad de microempresario */
/* Autor   : Fabiola Urqueta P.           Fecha: 12-01-2004                */
/* Modifica:                              Fecha:                           */
/***************************************************************************/
void CliRecActMEMPT(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   Q_HANDLE   *lista;
   tCliMEMP_CLASIFICACIONACTIVIDAD *MEMP_ClasificacionActividad;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_ClasificacionActividadTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de clasificacion de actividad microempresario." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      MEMP_ClasificacionActividad = (tCliMEMP_CLASIFICACIONACTIVIDAD *) QGetItem( lista, i);

      Fadd32( fml, CLI_CODIGO       ,  (char *) &MEMP_ClasificacionActividad->codigo,       0);
      Fadd32( fml, CLI_DESCRIPCION  ,            MEMP_ClasificacionActividad->descripcion,       0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista de Documentos de activiadades de microempresario*/
/* Autor   : Fabiola Urqueta P.           Fecha: 22-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDocuMEMPT(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   Q_HANDLE   *lista;
   tCliMEMP_DOCUMENTO *MEMP_Documento;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_DocumentoTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de documentos de actividad microempresario." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      MEMP_Documento = (tCliMEMP_DOCUMENTO *) QGetItem( lista, i);

      Fadd32( fml, CLI_CODIGO       ,  (char *) &MEMP_Documento->codigo,       0);
      Fadd32( fml, CLI_DESCRIPCION  ,            MEMP_Documento->descripcion,       0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista de Tipos de Negocios                        */
/* Autor   : Fabiola Urqueta P.           Fecha: 22-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecTipNegMET(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   int        transaccionGlobal;
   Q_HANDLE   *lista;
   tCliMEMP_TIPONEGOCIO *MEMP_TipoNegocio;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_TipoNegocioTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista de Tipos de negocios demicroempresario." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      MEMP_TipoNegocio = (tCliMEMP_TIPONEGOCIO *) QGetItem( lista, i);

      Fadd32( fml, CLI_CODIGO       ,  (char *) &MEMP_TipoNegocio->codigo,       0);
      Fadd32( fml, CLI_DESCRIPCION  ,            MEMP_TipoNegocio->descripcion,       0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/************************************************************************/
/* Objetivo: Inserta Actividad Vetado MicroEmpresario                   */
/* Autor   : Fabiola Urqueta P.           Fecha: 22-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsActVetMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_ACTIVIDADVETADA mEMP_ActividadVetado;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_ACTIVIDAD,             0, (char *)&mEMP_ActividadVetado.actividad, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarMEMP_ActividadVetada(&mEMP_ActividadVetado);
   if(sts != SQL_SUCCESS && sts != SQL_DUPLICATE_KEY)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta actividad vetado microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertarMEMP_ActividadVetada.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/* Objetivo: Elimina Actividad vetado  microempresario                  */
/* Autor   : Fabiola Urqueta P.           Fecha: 22-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmActVetMEP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliMEMP_ACTIVIDADVETADA mEMP_ActividadVetado;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_ACTIVIDAD, 0, (char *)&mEMP_ActividadVetado.actividad, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminarMEMP_ActividadVetada(&mEMP_ActividadVetado);
   if(sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar actividad vetado de microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista Actividad Vetado de microempresario         */
/* Autor   : Fabiola Urqueta P.           Fecha: 22-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecActVetMET(TPSVCINFO *rqst)
{
   FBFR32                          *fml;
   int                             transaccionGlobal;
   Q_HANDLE                        *lista;
   tCliMEMP_ACTIVIDADVETADA        *mEMP_ActividadVetado;

   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMEMP_ActividadVetadaTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      QDelete(lista);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar lista actividad vetado de  microEmpresario." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      mEMP_ActividadVetado  = (tCliMEMP_ACTIVIDADVETADA *) QGetItem( lista, i);
      Fadd32(fml, CLI_ACTIVIDAD,             (char *)&mEMP_ActividadVetado->actividad, 0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta una MicroEmpresa                                   */
/* Autor   : Fabiola Urqueta P.           Fecha: 23-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsMicroEmp(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliCLIENTEMICROEMPRESA clienteMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_RUTX, 0, (char *)&clienteMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_DVX, 0, clienteMicroEmpresa.dv, 0);
   sts |= Fget32(fml, CLI_RUBRO,0, (char *)&clienteMicroEmpresa.rubro, 0);
   sts |= Fget32(fml, CLI_TIPONEGOCIO,0, (char *)&clienteMicroEmpresa.tipoNegocio, 0);
   sts |= Fget32(fml, CLI_CLASIFICACIONACTIVIDAD,0, (char *)&clienteMicroEmpresa.clasificacionActividad, 0);
   sts |= Fget32(fml, CLI_NUMEROEMPLEADO,0, (char *)&clienteMicroEmpresa.numeroEmpleado, 0);
   sts |= Fget32(fml, CLI_NUMEROFAMILIARES,0, (char *)&clienteMicroEmpresa.numeroFamiliares, 0);
   sts |= Fget32(fml, CLI_NOMBREEMPRESA,0, clienteMicroEmpresa.NombreEmpresa, 0);
   sts |= Fget32(fml, CLI_NOMBRE_FANTASIA_EMPRESA,0, clienteMicroEmpresa.nombreFantasiaEmpresa, 0);
   sts |= Fget32(fml, CLI_FECHAINICIOACTIVIDAD,0, clienteMicroEmpresa.fechaInicioActividad, 0);
   sts |= Fget32(fml, CLI_INGRESOSNEGOCIOS,0, (char *)&clienteMicroEmpresa.ingresosNegocios, 0);
   sts |= Fget32(fml, CLI_INGRESOSFUERANEGOCIO,0, (char *)&clienteMicroEmpresa.ingresoFueraNegocio, 0);
   sts |= Fget32(fml, CLI_INGRESO_LIQUIDO_NEGOCIO,0, (char *)&clienteMicroEmpresa.ingresoLiquidoNegocio, 0);
   sts |= Fget32(fml, CLI_RUTREPRESENTANTE,0, (char *)&clienteMicroEmpresa.rutRepresentante, 0);
   sts |= Fget32(fml, CLI_MONTODEPOSITOAPLAZO,0, (char *)&clienteMicroEmpresa.montoDepositoaPlazo, 0);
   sts |= Fget32(fml, CLI_MONTOCUENTAAHORRO,0, (char *)&clienteMicroEmpresa.montoCuentaAhorro, 0);
   sts |= Fget32(fml, CLI_MONTOVEHICULOPARTICULAR,0, (char *)&clienteMicroEmpresa.montoVehiculoParticular, 0);
   sts |= Fget32(fml, CLI_MONTOVEHICULOTRABAJO,0, (char *)&clienteMicroEmpresa.montoVehiculoTrabajo, 0);
   sts |= Fget32(fml, CLI_MONTOMAQUINAEQUIPOS,0, (char *)&clienteMicroEmpresa.montoMaquinaEquipos, 0);
   sts |= Fget32(fml, CLI_MONTOMUEBLESUTILES,0, (char *)&clienteMicroEmpresa.montoMueblesUtiles, 0);
   sts |= Fget32(fml, CLI_MONTOLOCALINMUEBLES,0, (char *)&clienteMicroEmpresa.montoLocalInmuebles, 0);
   sts |= Fget32(fml, CLI_MONTOOTROSBIENES, 0,(char *)&clienteMicroEmpresa.montoOtrosBienes, 0);

   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertarclienteMicroEmpresa(&clienteMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta un cliente microEmpresa." , 0);
     userlog("%s: Error en funcion SqlCliInsertarclienteMicroEmpresa.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/* Objetivo: Modifica una MicroEmpresa                                  */
/* Autor   : Boris Contreras Mac-lean     Fecha: 15-04-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModMicroEmp(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliCLIENTEMICROEMPRESA clienteMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = Fget32(fml, CLI_RUTX, 0, (char *)&clienteMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_DVX, 0, clienteMicroEmpresa.dv, 0);
   sts |= Fget32(fml, CLI_RUBRO,0, (char *)&clienteMicroEmpresa.rubro, 0);
   sts |= Fget32(fml, CLI_TIPONEGOCIO,0, (char *)&clienteMicroEmpresa.tipoNegocio, 0);
   sts |= Fget32(fml, CLI_CLASIFICACIONACTIVIDAD,0, (char *)&clienteMicroEmpresa.clasificacionActividad, 0);
   sts |= Fget32(fml, CLI_NUMEROEMPLEADO,0, (char *)&clienteMicroEmpresa.numeroEmpleado, 0);
   sts |= Fget32(fml, CLI_NUMEROFAMILIARES,0, (char *)&clienteMicroEmpresa.numeroFamiliares, 0);
   sts |= Fget32(fml, CLI_NOMBREEMPRESA,0, clienteMicroEmpresa.NombreEmpresa, 0);
   sts |= Fget32(fml, CLI_NOMBRE_FANTASIA_EMPRESA,0, clienteMicroEmpresa.nombreFantasiaEmpresa, 0);
   sts |= Fget32(fml, CLI_FECHAINICIOACTIVIDAD,0, clienteMicroEmpresa.fechaInicioActividad, 0);
   sts |= Fget32(fml, CLI_INGRESOSNEGOCIOS,0, (char *)&clienteMicroEmpresa.ingresosNegocios, 0);
   sts |= Fget32(fml, CLI_INGRESO_LIQUIDO_NEGOCIO,0, (char *)&clienteMicroEmpresa.ingresoLiquidoNegocio, 0);
   sts |= Fget32(fml, CLI_INGRESOSFUERANEGOCIO,0, (char *)&clienteMicroEmpresa.ingresoFueraNegocio, 0);
   sts |= Fget32(fml, CLI_RUTREPRESENTANTE,0, (char *)&clienteMicroEmpresa.rutRepresentante, 0);
   sts |= Fget32(fml, CLI_MONTODEPOSITOAPLAZO,0, (char *)&clienteMicroEmpresa.montoDepositoaPlazo, 0);
   sts |= Fget32(fml, CLI_MONTOCUENTAAHORRO,0, (char *)&clienteMicroEmpresa.montoCuentaAhorro, 0);
   sts |= Fget32(fml, CLI_MONTOVEHICULOPARTICULAR,0, (char *)&clienteMicroEmpresa.montoVehiculoParticular, 0);
   sts |= Fget32(fml, CLI_MONTOVEHICULOTRABAJO,0, (char *)&clienteMicroEmpresa.montoVehiculoTrabajo, 0);
   sts |= Fget32(fml, CLI_MONTOMAQUINAEQUIPOS,0, (char *)&clienteMicroEmpresa.montoMaquinaEquipos, 0);
   sts |= Fget32(fml, CLI_MONTOMUEBLESUTILES,0, (char *)&clienteMicroEmpresa.montoMueblesUtiles, 0);
   sts |= Fget32(fml, CLI_MONTOLOCALINMUEBLES,0, (char *)&clienteMicroEmpresa.montoLocalInmuebles, 0);
   sts |= Fget32(fml, CLI_MONTOOTROSBIENES, 0,(char *)&clienteMicroEmpresa.montoOtrosBienes, 0);

   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliModificarclienteMicroEmpresa(&clienteMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta un cliente microEmpresa." , 0);
     userlog("%s: Error en funcion SqlCliModificarclienteMicroEmpresa.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/*************************************************************************/
/* Objetivo: Recupera una MicroEmpresa                                  */
/* Autor   : Fabiola Urqueta P.           Fecha: 23-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecMicroEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLIENTEMICROEMPRESA clienteMicroEmpresa;
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   sts = Fget32(fml, CLI_RUTX, 0, (char *)&clienteMicroEmpresa.rutEmpresa, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperarclienteMicroEmpresa(&clienteMicroEmpresa);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no se encuentra como cliente micro empresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar el rut de empresa de cliente micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }


   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_RUTX, (char *)&clienteMicroEmpresa.rutEmpresa, 0);
   Fadd32(fml, CLI_DVX, clienteMicroEmpresa.dv, 0);
   Fadd32(fml, CLI_RUBRO, (char *)&clienteMicroEmpresa.rubro, 0);
   Fadd32(fml, CLI_TIPONEGOCIO, (char *)&clienteMicroEmpresa.tipoNegocio, 0);
   Fadd32(fml, CLI_CLASIFICACIONACTIVIDAD, (char *)&clienteMicroEmpresa.clasificacionActividad, 0);
   Fadd32(fml, CLI_NUMEROEMPLEADO, (char *)&clienteMicroEmpresa.numeroEmpleado, 0);
   Fadd32(fml, CLI_NUMEROFAMILIARES, (char *)&clienteMicroEmpresa.numeroFamiliares, 0);

   Fadd32(fml, CLI_NOMBREEMPRESA, clienteMicroEmpresa.NombreEmpresa, 0);
   Fadd32(fml, CLI_NOMBRE_FANTASIA_EMPRESA, clienteMicroEmpresa.nombreFantasiaEmpresa, 0);

   Fadd32(fml, CLI_FECHAINICIOACTIVIDAD, clienteMicroEmpresa.fechaInicioActividad, 0);
   Fadd32(fml, CLI_INGRESOSNEGOCIOS, (char *)&clienteMicroEmpresa.ingresosNegocios, 0);
   Fadd32(fml, CLI_INGRESO_LIQUIDO_NEGOCIO, (char *)&clienteMicroEmpresa.ingresoLiquidoNegocio, 0);
   Fadd32(fml, CLI_INGRESOSFUERANEGOCIO, (char *)&clienteMicroEmpresa.ingresoFueraNegocio, 0);
   Fadd32(fml, CLI_RUTREPRESENTANTE, (char *)&clienteMicroEmpresa.rutRepresentante, 0);
   Fadd32(fml, CLI_MONTODEPOSITOAPLAZO, (char *)&clienteMicroEmpresa.montoDepositoaPlazo, 0);
   Fadd32(fml, CLI_MONTOCUENTAAHORRO, (char *)&clienteMicroEmpresa.montoCuentaAhorro, 0);
   Fadd32(fml, CLI_MONTOVEHICULOPARTICULAR, (char *)&clienteMicroEmpresa.montoVehiculoParticular, 0);
   Fadd32(fml, CLI_MONTOVEHICULOTRABAJO, (char *)&clienteMicroEmpresa.montoVehiculoTrabajo, 0);
   Fadd32(fml, CLI_MONTOMAQUINAEQUIPOS, (char *)&clienteMicroEmpresa.montoMaquinaEquipos, 0);
   Fadd32(fml, CLI_MONTOMUEBLESUTILES, (char *)&clienteMicroEmpresa.montoMueblesUtiles, 0);
   Fadd32(fml, CLI_MONTOLOCALINMUEBLES, (char *)&clienteMicroEmpresa.montoLocalInmuebles, 0);
   Fadd32(fml, CLI_MONTOOTROSBIENES, (char *)&clienteMicroEmpresa.montoOtrosBienes, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta una direccion de MicroEmpresario                   */
/* Autor   : Fabiola Urqueta P.           Fecha: 26-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDirecMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliDIRECCIONMICROEMPRESA direccionMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&direccionMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccionMicroEmpresa.tipoDireccion, 0);
   sts |= Fget32(fml, CLI_CIUDAD, 0, (char *)&direccionMicroEmpresa.ciudad, 0);
   sts |= Fget32(fml, CLI_COMUNA, 0, (char *)&direccionMicroEmpresa.comuna, 0);
   sts |= Fget32(fml, CLI_CODIGO_POSTAL, 0, direccionMicroEmpresa.codigoPostal, 0);

   if (Foccur32(fml, CLI_CALLE_NUMERO) > 0)
    sts |= Fget32(fml, CLI_CALLE_NUMERO, 0, direccionMicroEmpresa.calleNumero, 0);
   else
    direccionMicroEmpresa.calleNumero[0] = '\0';

   if (Foccur32(fml, CLI_DEPARTAMENTO) > 0)
    sts |= Fget32(fml, CLI_DEPARTAMENTO, 0, direccionMicroEmpresa.departamento, 0);
   else
    direccionMicroEmpresa.departamento[0] = '\0';

   if (Foccur32(fml,CLI_VILLA_POBLACION) > 0)
    sts |= Fget32(fml, CLI_VILLA_POBLACION, 0, direccionMicroEmpresa.villaPoblacion, 0);
   else
    direccionMicroEmpresa.villaPoblacion[0] = '\0';

   if (Foccur32(fml,CLI_NUMERODIRECCION) > 0)
    sts |= Fget32(fml, CLI_NUMERODIRECCION, 0, (char *)&direccionMicroEmpresa.numero, 0);
   else
    direccionMicroEmpresa.numero = -1;


   if (Foccur32(fml,CLI_NUMEROEDITADO) > 0)
    sts |= Fget32(fml, CLI_NUMEROEDITADO, 0, direccionMicroEmpresa.numeroEditado, 0);
   else
    direccionMicroEmpresa.numeroEditado[0] = '\0';

   if (Foccur32(fml,CLI_OBSERVACIONES) > 0)
    sts |= Fget32(fml, CLI_OBSERVACIONES, 0, direccionMicroEmpresa.observaciones, 0);
   else
    direccionMicroEmpresa.observaciones[0] = '\0';


   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertardireccionMicroEmpresa(&direccionMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta direccion de microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertardireccionMicroEmpresa.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/* Objetivo: Elimina una direccion de microempresario                   */
/* Autor   : Fabiola Urqueta P.           Fecha: 26-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmDirecMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliDIRECCIONMICROEMPRESA direccionMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&direccionMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccionMicroEmpresa.tipoDireccion, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminardireccionMicroEmpresa(&direccionMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar direccion microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modifica  una direccion de microempresario                 */
/* Autor   : Fabiola Urqueta P.           Fecha: 26-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModDirecMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliDIRECCIONMICROEMPRESA direccionMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&direccionMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccionMicroEmpresa.tipoDireccion, 0);
   sts |= Fget32(fml, CLI_CIUDAD, 0, (char *)&direccionMicroEmpresa.ciudad, 0);
   sts |= Fget32(fml, CLI_COMUNA, 0, (char *)&direccionMicroEmpresa.comuna, 0);
   sts |= Fget32(fml, CLI_CODIGO_POSTAL, 0, direccionMicroEmpresa.codigoPostal, 0);

   if (Foccur32(fml, CLI_CALLE_NUMERO) > 0)
    sts |= Fget32(fml, CLI_CALLE_NUMERO, 0, direccionMicroEmpresa.calleNumero, 0);
   else
    direccionMicroEmpresa.calleNumero[0] = '\0';

   if (Foccur32(fml, CLI_DEPARTAMENTO) > 0)
    sts |= Fget32(fml, CLI_DEPARTAMENTO, 0, direccionMicroEmpresa.departamento, 0);
   else
    direccionMicroEmpresa.departamento[0] = '\0';

   if (Foccur32(fml,CLI_VILLA_POBLACION) > 0)
    sts |= Fget32(fml, CLI_VILLA_POBLACION, 0, direccionMicroEmpresa.villaPoblacion, 0);
   else
    direccionMicroEmpresa.villaPoblacion[0] = '\0';

   if (Foccur32(fml,CLI_NUMERODIRECCION) > 0)
    sts |= Fget32(fml, CLI_NUMERODIRECCION, 0,(char *)&direccionMicroEmpresa.numero, 0);
   else
    direccionMicroEmpresa.numero = -1;

   if (Foccur32(fml,CLI_NUMEROEDITADO) > 0)
    sts |= Fget32(fml, CLI_NUMEROEDITADO, 0, direccionMicroEmpresa.numeroEditado, 0);
   else
    direccionMicroEmpresa.numeroEditado[0] = '\0';

   if (Foccur32(fml,CLI_OBSERVACIONES) > 0)
    sts |= Fget32(fml, CLI_OBSERVACIONES, 0, direccionMicroEmpresa.observaciones, 0);
   else
    direccionMicroEmpresa.observaciones[0] = '\0';


   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliModificardireccionMicroEmpresa(&direccionMicroEmpresa);
   if(sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo modificar direccion microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if(sts == SQL_NOT_FOUND)
   {
     sts = SqlCliInsertardireccionMicroEmpresa(&direccionMicroEmpresa);
     if(sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo modificar la direccion de microEmpresario." , 0);
        userlog("%s: Error en funcion SqlCliInsertardireccionMicroEmpresa.", rqst->name);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera una direccion de MicroEmpresa                     */
/* Autor   : Fabiola Urqueta P.           Fecha: 26-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDirecMEMP(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliDIRECCIONMICROEMPRESA direccionMicroEmpresa;
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&direccionMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_DIRECCION, 0, (char *)&direccionMicroEmpresa.tipoDireccion, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperardireccionMicroEmpresa(&direccionMicroEmpresa);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no se encuentra en direcciones de micro empresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informacion de direccion de cliente micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_RUTX,(char *)&direccionMicroEmpresa.rutEmpresa, 0);
   Fadd32(fml, CLI_TIPO_DIRECCION,(char *)&direccionMicroEmpresa.tipoDireccion, 0);
   Fadd32(fml, CLI_CALLE_NUMERO, direccionMicroEmpresa.calleNumero, 0);
   Fadd32(fml, CLI_DEPARTAMENTO, direccionMicroEmpresa.departamento, 0);
   Fadd32(fml, CLI_VILLA_POBLACION, direccionMicroEmpresa.villaPoblacion, 0);
   Fadd32(fml, CLI_CIUDAD,(char *)&direccionMicroEmpresa.ciudad, 0);
   Fadd32(fml, CLI_COMUNA,(char *)&direccionMicroEmpresa.comuna, 0);
   Fadd32(fml, CLI_CODIGO_POSTAL, direccionMicroEmpresa.codigoPostal, 0);
   Fadd32(fml, CLI_NUMERODIRECCION,(char *)&direccionMicroEmpresa.numero, 0);
   Fadd32(fml, CLI_NUMEROEDITADO,  direccionMicroEmpresa.numeroEditado, 0);
   Fadd32(fml, CLI_OBSERVACIONES,  direccionMicroEmpresa.observaciones, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista de direccion de microempresario             */
/* Autor   : Fabiola Urqueta P.           Fecha: 26-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDirecMEMT(TPSVCINFO *rqst)
{
   FBFR32                          *fml;
   int                             transaccionGlobal;
   Q_HANDLE                        *lista;
   tCliDIRECCIONMICROEMPRESA       *direccionMicroEmpresa;
   long                            rutEmpresa;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&rutEmpresa, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperardireccionMicroEmpresaTodos(lista, rutEmpresa);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no se encuentra en direcciones de micro empresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informacion de direccion de cliente micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      direccionMicroEmpresa  = (tCliDIRECCIONMICROEMPRESA *) QGetItem( lista, i);
      Fadd32(fml, CLI_RUTX,          (char *)&direccionMicroEmpresa->rutEmpresa, 0);
      Fadd32(fml, CLI_TIPO_DIRECCION,(char *)&direccionMicroEmpresa->tipoDireccion, 0);
      Fadd32(fml, CLI_CALLE_NUMERO,           direccionMicroEmpresa->calleNumero, 0);
      Fadd32(fml, CLI_DEPARTAMENTO,           direccionMicroEmpresa->departamento, 0);
      Fadd32(fml, CLI_VILLA_POBLACION,        direccionMicroEmpresa->villaPoblacion, 0);
      Fadd32(fml, CLI_CIUDAD,         (char *)&direccionMicroEmpresa->ciudad, 0);
      Fadd32(fml, CLI_COMUNA,         (char *)&direccionMicroEmpresa->comuna, 0);
      Fadd32(fml, CLI_CODIGO_POSTAL,           direccionMicroEmpresa->codigoPostal, 0);
      Fadd32(fml, CLI_NUMERODIRECCION,(char *)&direccionMicroEmpresa->numero, 0);
      Fadd32(fml, CLI_NUMEROEDITADO,           direccionMicroEmpresa->numeroEditado, 0);
      Fadd32(fml, CLI_OBSERVACIONES,           direccionMicroEmpresa->observaciones, 0);

   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Inserta un telefono MicroEmpresario                        */
/* Autor   : Fabiola Urqueta P.           Fecha: 27-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsTelefMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliTELEFONOMICROEMPRESA telefonoMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&telefonoMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefonoMicroEmpresa.tipoTelefono, 0);
   sts |= Fget32(fml, CLI_NUMERO, 0, telefonoMicroEmpresa.numero, 0);
   sts |= Fget32(fml, CLI_CODIGO_AREA, 0, (char *)&telefonoMicroEmpresa.codigoArea, 0);

   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliInsertartelefonoMicroEmpresa(&telefonoMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta telefono de microEmpresario." , 0);
     userlog("%s: Error en funcion SqlCliInsertartelefonoMicroEmpresa.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/* Objetivo: Elimina un telefono de microempresario                     */
/* Autor   : Fabiola Urqueta P.           Fecha: 27-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmTelefMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliTELEFONOMICROEMPRESA telefonoMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&telefonoMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefonoMicroEmpresa.tipoTelefono, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliEliminartelefonoMicroEmpresa(&telefonoMicroEmpresa);
   if(sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo eliminar telefono microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modifica  un telefono de  microempresario                  */
/* Autor   : Fabiola Urqueta P.           Fecha: 27-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModTelefMEMP(TPSVCINFO *rqst)
{
   FBFR32    *fml;
   int       transaccionGlobal;
   tCliTELEFONOMICROEMPRESA telefonoMicroEmpresa;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&telefonoMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefonoMicroEmpresa.tipoTelefono, 0);
   sts |= Fget32(fml, CLI_NUMERO, 0, telefonoMicroEmpresa.numero, 0);
   sts |= Fget32(fml, CLI_CODIGO_AREA, 0, (char *)&telefonoMicroEmpresa.codigoArea, 0);

   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliModificartelefonoMicroEmpresa(&telefonoMicroEmpresa);
   if(sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo modificar telefono microEmpresario." , 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (sts == SQL_NOT_FOUND)
   {
     sts = SqlCliInsertartelefonoMicroEmpresa(&telefonoMicroEmpresa);
     if(sts != SQL_SUCCESS)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo inserta telefono de microEmpresario." , 0);
       userlog("%s: Error en funcion SqlCliInsertartelefonoMicroEmpresa.", rqst->name);
       TRX_ABORT(transaccionGlobal);
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera un telefono de MicroEmpresa                       */
/* Autor   : Fabiola Urqueta P.           Fecha: 27-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecTelefMEMP(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTELEFONOMICROEMPRESA telefonoMicroEmpresa;
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&telefonoMicroEmpresa.rutEmpresa, 0);
   sts |= Fget32(fml, CLI_TIPO_TELEFONO, 0, (char *)&telefonoMicroEmpresa.tipoTelefono, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperartelefonoMicroEmpresa(&telefonoMicroEmpresa);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no se encuentra en telefono de micro empresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informacion de telefono de cliente micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_RUTX,(char *)&telefonoMicroEmpresa.rutEmpresa, 0);
   Fadd32(fml, CLI_TIPO_TELEFONO,(char *)&telefonoMicroEmpresa.tipoTelefono, 0);
   Fadd32(fml, CLI_NUMERO, telefonoMicroEmpresa.numero, 0);
   Fadd32(fml, CLI_CODIGO_AREA, (char *)&telefonoMicroEmpresa.codigoArea, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recupera lista de telefonos de microempresario             */
/* Autor   : Fabiola Urqueta P.           Fecha: 27-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecTelefMEMT(TPSVCINFO *rqst)
{
   FBFR32                          *fml;
   int                             transaccionGlobal;
   Q_HANDLE                        *lista;
   tCliTELEFONOMICROEMPRESA        *telefonoMicroEmpresa;
   long                            rutEmpresa;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_RUTX, 0, (char *)&rutEmpresa, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }


   sts = SqlCliRecuperartelefonoMicroEmpresaTodos(lista, rutEmpresa);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El cliente no se encuentra en telefono de micro empresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informacion de telefono de cliente micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      telefonoMicroEmpresa  = (tCliTELEFONOMICROEMPRESA *) QGetItem( lista, i);
      Fadd32(fml, CLI_RUTX,(char *)&telefonoMicroEmpresa->rutEmpresa, 0);
      Fadd32(fml, CLI_TIPO_TELEFONO,(char *)&telefonoMicroEmpresa->tipoTelefono, 0);
      Fadd32(fml, CLI_NUMERO, telefonoMicroEmpresa->numero, 0);
      Fadd32(fml, CLI_CODIGO_AREA,( char *)&telefonoMicroEmpresa->codigoArea, 0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recupera un Rubro de microEmpresa                          */
/* Autor   : Fabiola Urqueta P.           Fecha: 30-01-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecRubroMEMP(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliMEMP_RUBRO  MEMP_Rubro;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   sts = Fget32(fml, CLI_RUBRO, 0, (char *)&MEMP_Rubro.codigo, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperarMEMP_Rubro(&MEMP_Rubro);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El rubro no se encuentra. ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar el rubro de micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_RUBRO,(char *)&MEMP_Rubro.codigo, 0);
   Fadd32(fml, CLI_DESCRIPCION, MEMP_Rubro.descripcion, 0);
   Fadd32(fml, CLI_RUBRO_SBIF,(char *)&MEMP_Rubro.rubroSBIF, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recupera una Actividad de microEmpresa                     */
/* Autor   : Fabiola Urqueta P.           Fecha: 16-02-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecActivMEMP(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliMEMP_CLASIFICACIONACTIVIDAD mEMP_ClasificacionActividad;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   sts = Fget32(fml, CLI_CODIGO, 0, (char *)&mEMP_ClasificacionActividad.codigo, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperarMEMP_ClasificacionActividad(&mEMP_ClasificacionActividad);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La actividad no se encuentra.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la actividad de micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_CODIGO,(char *)&mEMP_ClasificacionActividad.codigo, 0);
   Fadd32(fml, CLI_DESCRIPCION, mEMP_ClasificacionActividad.descripcion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recupera un documento de microEmpresa                      */
/* Autor   : Fabiola Urqueta P.           Fecha: 17-02-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDocumMEMP(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliMEMP_DOCUMENTO mEMP_Documento;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   sts = Fget32(fml, CLI_CODIGO, 0, (char *)&mEMP_Documento.codigo, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   sts = SqlCliRecuperarMEMP_Documento(&mEMP_Documento);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El documento no se encuentra.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar el documento de micro empresa", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_CODIGO,(char *)&mEMP_Documento.codigo, 0);
   Fadd32(fml, CLI_DESCRIPCION, mEMP_Documento.descripcion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Recupera documentos por actividad de la microEmpresa       */
/* Autor   : Fabiola Urqueta P.           Fecha: 17-02-2004             */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDocActMEP(TPSVCINFO *rqst)
{
   FBFR32                          *fml;
   int                             transaccionGlobal;
   Q_HANDLE                        *lista;
   tCliMEMP_RDOCUMENTOACTIVIDAD    *mEMP_RDocumentoActividad;
   short      clasificacionActividad;
   int        i=0;
   long       sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts  = Fget32(fml, CLI_CLASIFICACIONACTIVIDAD, 0, (char *)&clasificacionActividad, 0);
   if (sts == -1)
     {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en el ingreso de los datos." , 0);
     userlog("%s: Error en el ingreso de fml.", rqst->name);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

   if((lista = (Q_HANDLE *) QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, SQL_MEMORY, (char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperardocumentoPorActividadMicroEmpresaTodos(lista, clasificacionActividad);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe documentos asociados a la actividad de la microEmpresa", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informacion documentos de una actividad de microEmpresa",0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)1, (short)0, (short)1, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for(i=0;i<lista->count;i++)
   {
      mEMP_RDocumentoActividad = (tCliMEMP_RDOCUMENTOACTIVIDAD *) QGetItem( lista, i);
      Fadd32(fml, CLI_DOCUMENTO,(char *)&mEMP_RDocumentoActividad->documento, 0);
      Fadd32(fml, CLI_CLASIFICACIONACTIVIDAD,(char *)&mEMP_RDocumentoActividad->clasificacionActividad, 0);
   }

   QDelete(lista);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/********************************************* FIN MICROEMPRESARIO ***************************************************/
/*********************************************************************************************************************/


/************************************************************************/
/* Objetivo: Recuperar firma de cliente                                 */
/* Autor   : Elena López Salas                  Fecha: 19-01-2004       */
/* Modifica:                                                            */
/************************************************************************/
void CliValExiFirma(TPSVCINFO *rqst)
{
   FBFR32      *fml;
   int         transaccionGlobal;
   long        sts;
   short       i;
   long        rutCliente;
   short       nroRegistros;

   /******   Buffer de entrada   ******/

   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT,          0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarCantidadFirmaCliente(rutCliente, &nroRegistros);

   if (sts != SQL_SUCCESS)
   {
     Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la firma", 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);

   Fadd32(fml, CLI_OCURRENCIAS, (char *)&nroRegistros, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************************************************/
/* Objetivo: Obtener segmento del cliento y almacenarlo                  */
/* Autor   :                                          Fecha: 13-04-2005  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliRecModSegCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliCLIENTE cliente;
   tCliRENTA renta;
   tCliACTIVIDAD actividad;
   tCliSEGMENTOCLIENTE segmentoCliente;
   long sts;
   int nAnoCliente,  nAnoProceso, edad;
   char sAnoCliente[4 + 1], sAnoProceso[4 + 1];
   char fecha[8 + 1];
   short segmento;
   short diffDias;
   short tipoCliente;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&cliente.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

/********** ContinuidadHoraria [INI] **********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog(" Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
/********** ContinuidadHoraria [FIN] **********/

   /* Rcuperar sexo, */
   sts = SqlCliRecuperarCliente(&cliente);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   segmento = -1;

   tipoCliente = 0;
   sts = SqlCliRecuperarClienteEspecial(&cliente, &tipoCliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar cliente", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

 /*   userlog("tipoCliente %d\n", tipoCliente); */
   if (tipoCliente > 0)
   {
      if (tipoCliente == 1) segmento = 20;

      if (tipoCliente == 2) segmento = 30;
   }

   if (segmento < 0)
   {
     renta.rutCliente = cliente.rut;
     renta.tipoMontoRenta = 2;

     sts = SqlCliRecuperarUnaRenta(&renta);
     if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar renta", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
     if (sts == SQL_NOT_FOUND)
     {
        renta.rutCliente = cliente.rut;
        renta.tipoMontoRenta = 1;

        sts = SqlCliRecuperarUnaRenta(&renta);
        if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar renta", 0);
           TRX_ABORT(transaccionGlobal);
           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        if (sts == SQL_NOT_FOUND)
        {
           renta.rutCliente = cliente.rut;
           renta.tipoMontoRenta = 3;

           sts = SqlCliRecuperarUnaRenta(&renta);
           if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
           {
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar renta", 0);
              TRX_ABORT(transaccionGlobal);
              tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
           }

           if (sts == SQL_NOT_FOUND) renta.monto = 0;
        }

     }

     actividad.rutCliente = cliente.rut;
     sts = SqlCliRecuperarActividadLaboral(&actividad);
     if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error recuperar actividad", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
     if (sts == SQL_NOT_FOUND)
       actividad.actividad = 0;

     strncpy(sAnoCliente, cliente.fechaNacimiento + 4, 4);
     sAnoCliente[4] = '\0';
     strncpy(sAnoProceso, gFechaProceso + 4, 4);
     sAnoProceso[4] = '\0';

     nAnoCliente = atoi(sAnoCliente);
     nAnoProceso = atoi(sAnoProceso);

     edad = nAnoProceso - nAnoCliente;

/*
     userlog("...==> rut %li sexo %d edad %d renta %f actividad %d",
                     cliente.rut, cliente.sexo, edad, renta.monto, actividad.actividad);
*/
     /* Sexo  1:Masculino 2:Femenino */

     if (((cliente.sexo == 1) || (cliente.sexo == 2)) && (edad >= 21) && (renta.monto >= 1500000))
            segmento = 40;

     if ((segmento < 0) && (cliente.sexo == 2) && (edad >= 21) &&
        ((renta.monto >= 600000) && (renta.monto < 1500000)))
          segmento = 50;

     if ((segmento < 0) && (cliente.sexo == 2) && (edad >= 21) &&
         (actividad.actividad == 1)) /* Dueña de casa */
          segmento = 60;

     if ((segmento < 0) && (cliente.sexo == 1) && (edad >= 21) &&
         ((renta.monto >= 700000) && (renta.monto < 1500000)))
          segmento = 70;

     if ((segmento < 0) && ((cliente.sexo == 1) || (cliente.sexo == 2)) &&
         (edad >= 21) && (edad <= 29) && (renta.monto >= 0))
          segmento = 80;

     if ((segmento < 0) && ((cliente.sexo == 1) || (cliente.sexo == 2)) &&
         (edad >= 21) && (edad <= 29) && (renta.monto <= 0))
          segmento = 90;

     if ((segmento < 0) && ((cliente.sexo == 1) || (cliente.sexo == 2)) &&
         (edad >= 55) && (renta.monto >= 350000)  && (actividad.actividad == 10)) /* jubilado */
          segmento = 100;

     if ((segmento < 0) && ((cliente.sexo == 1) || (cliente.sexo == 2)) &&
         (edad >= 30) && (renta.monto >= 350000) )
          segmento = 110;

     if (segmento < 0)  segmento = 10;
   }

   segmentoCliente.rutCliente = cliente.rut;
   segmentoCliente.segmento = segmento;
   strcpy(segmentoCliente.fechaSegmentacion, gFechaProceso);

   strcpy(fecha, gFechaProceso);

   sts = SqlCliValidarMinimoTiempoSegmentoCliente(&segmentoCliente, 6, fecha, gFechaProceso);
                                                                 /* 6: (meses) en segmento actual*/
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar minimo tiempo", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
/*
   userlog(".....==>Segmento %d \n", segmento);
   userlog(".....==>Fecha %s \n", fecha);
   userlog(".....==>gFechaProceso %s \n", gFechaProceso);
*/
   sts = DiferenciaFechas(gFechaProceso, fecha, &diffDias);

/*   userlog(".....==>dias %d \n", diffDias); */

   if (diffDias >= 0)
   {

      sts = SqlCliInsertarsegmentoCliente(&segmentoCliente);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error grabar segmentacion cliente", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/*********************************************************************/
/*********************************************************************/
void CliRecRegTod(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   FBFR32 *fml2;
   short f=0;
   int j=0;
   int i=0;
   int transaccionGlobal=0;
   long sts=0;
   long  largo;
   Q_HANDLE *lista;
   tCliREGMOD *pRegMod;
   tCliREGMOD  regMod;
   tCliREGMOD  regMod1; 
   tCliDIRECCION direccion; 
   tCliTELEFONO telefono; 
   tCliCODIGODESCRIPCION tipoTelefono;
   tCliCODIGODESCRIPCION tipoDeDireccion;
   long rut; 
   short tipo;
   char numero[20 +1];
   char numero2[20 +1];
  
   short tipoDireccion;	
   char direccionCalleNumero[40+1];
   char direccionCalleNumero2[40+1];
   char departamento[8+1];
   char departamento2[8+1];
   char villaPoblacion[40+1];
   char villaPoblacion2[40 +1];
   char nombre[40 +1]; 
   short ciudad;
   short comuna;
   char codigoPostal[10+1];
   char codigoPostal2[10+1];
   long identificador;
   long identificadorAnterior;
   short ciudadAnterior=-1;
   short comunaAnterior=-1; 
   short tipoDireccionAnterior=-1;  
   char  ciudadDescripcion[50 + 1];
   char  comunaDescripcion[50 + 1]; 
   ciudadDescripcion[0] = '\0'; 
   short  PARCIAL_POR_IDENTIFICADOR=11;  

   memset((void *)&regMod,0,sizeof(tCliREGMOD));
   memset((void *)&regMod1,0,sizeof(tCliREGMOD)); 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal); 
    
   Fget32(fml, CLI_RUTX, 0, (char *)&regMod.rutCliente, 0);
  
   if (Foccur32(fml, CLI_CORRELATIVO) > 0)
   {  
      Fget32(fml, CLI_CORRELATIVO, 0, (char *)&regMod.correlativo, 0);
   }
   else
   {   regMod1.rutCliente= regMod.rutCliente;   
       sts = SqlCliMAXCorrelativo(&regMod1);
       if (sts != SQL_SUCCESS)
       {
         if (sts != SQL_NOT_FOUND)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar la clave de acceso del cliente", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
      } 
       regMod.correlativo= regMod1.correlativo; 
   }      

   if ((lista = QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear lista registro Modificacion.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name),(char *)fml, 0L, 0L);
   }
   sts = SqlCliRecuperarRegistroModificacionTodos(&regMod, lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registro Modificacion", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registro Modificacion", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   tpfree((char *)fml);
   fml = NewFml32((long)lista->count, (short)6, (short)4, (short)3, (long)228);
   if (fml == NULL)
   {
      userlog("%s No es posible realizar la accion solicitada. (NewFml32).", rqst->name);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No es posible realizar la accion solicitada", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(rqst->name, "CliRecRegModDir") == 0)
   {
   for (i=0; i<lista->count;i++)
   {
      pRegMod = (tCliREGMOD *) QGetItem( lista, i);
     
      if (pRegMod->tipoModificacion == CLI_MODIFICACION_DIRECCION ||
          pRegMod->tipoModificacion == CLI_ELIMINACION_DIRECCION)
      {
         Fadd32(fml, CLI_CORRELATIVO, (char *)&pRegMod->correlativo, 0);
         Fadd32(fml, CLI_FECHA, pRegMod->fecha, 0);
         Fadd32(fml, CLI_USUARIO, (char *)&pRegMod->usuario, 0);
         Fadd32(fml, CLI_SUCURSAL,(char *)&pRegMod->sucursal, 0);
         Fadd32(fml, CLI_TIPO_MODIFICACION,(char *)&pRegMod->tipoModificacion, 0);
         Fadd32(fml, CLI_RUTX, (char *)&pRegMod->rutCliente, 0);
         identificador= pRegMod->usuario; 
         if( identificador!= identificadorAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, HRM_TIPO, (char *)&PARCIAL_POR_IDENTIFICADOR, 0);
            Fadd32(fml2, HRM_IDENTIFICADOR_USUARIO, (char *)&identificador, 0);
          
            if (tpcall("HrmRecUsuarios", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(nombre, "S/A");
            else
                Fget32(fml2, HRM_NOMBRE, 0, nombre, 0);
            
             tpfree((char *)fml2);
            identificadorAnterior = identificador;
         }

         f_strcpylng(&rut,pRegMod->modificacion,9);
	 f_strcpysht(&tipoDireccion ,pRegMod->modificacion+9, 4);
         direccion.rutCliente= rut;
         direccion.tipoDireccion= tipoDireccion;

         if(tipoDireccion != tipoDireccionAnterior)
         {
            tipoDeDireccion.codigo = tipoDireccion;
            sts = SqlCliRecuperarTipoDireccion(&tipoDeDireccion);
            if (sts != SQL_SUCCESS)
            {
               strcpy(tipoDeDireccion.descripcion, "Tipo inexistente");
            }
            tipoDireccionAnterior = tipoDireccion; 
         } 
          
         f_strcpystr(direccionCalleNumero,pRegMod->modificacion+13, 40);
         direccionCalleNumero[40] = '\0';       
	 strcpy(direccionCalleNumero2, direccionCalleNumero);

         f_strcpystr(departamento,pRegMod->modificacion+53, 8);
         departamento[8] = '\0';
         strcpy(departamento2, departamento);

         f_strcpystr(villaPoblacion,pRegMod->modificacion+61, 40);
         villaPoblacion[40] = '\0';
         strcpy(villaPoblacion2, villaPoblacion);

         f_strcpysht(&ciudad , pRegMod->modificacion+101, 4);
         f_strcpysht(&comuna , pRegMod->modificacion+105, 4);

         f_strcpystr(codigoPostal,pRegMod->modificacion+109, 10);
         codigoPostal[10] = '\0';
         strcpy(codigoPostal2, codigoPostal);

	 if(ciudad != ciudadAnterior)
	 {
	    fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
	    Fadd32(fml2, SGT_CODIGO, (char *)&ciudad, 0);

            if (tpcall("SgtRecCiudad", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(ciudadDescripcion, "Ciudad inexistente");
            else
               Fget32(fml2, SGT_DESCRIPCION, 0, ciudadDescripcion, 0);
            tpfree((char *)fml2);
            ciudadAnterior = ciudad;
	 }

         if(comuna != comunaAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, SGT_CODIGO, (char *)&comuna, 0);

            if (tpcall("SgtRecComuna", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(comunaDescripcion, "Comuna inexistente");
            else
               Fget32(fml2, SGT_DESCRIPCION, 0, comunaDescripcion, 0);
            tpfree((char *)fml2);
            comunaAnterior = comuna;
         }
         Fadd32(fml,CLI_TIPO_DIRECCION,(char *)&tipoDeDireccion.codigo, 0);
         Fadd32(fml,CLI_DIRECCION,tipoDeDireccion.descripcion, 0);
         Fadd32(fml,CLI_CALLE_NUMERO,direccionCalleNumero2, 0);
         Fadd32(fml,CLI_DEPARTAMENTO,departamento2, 0);
         Fadd32(fml,CLI_VILLA_POBLACION,villaPoblacion2, 0);
         Fadd32(fml,CLI_CIUDAD,(char *)&ciudad, 0);
         Fadd32(fml,CLI_DESCRIPCION_CIUDAD,ciudadDescripcion, 0); 
         Fadd32(fml,CLI_DESCRIPCION_COMUNA,comunaDescripcion, 0);
         Fadd32(fml,CLI_NOMBRE,nombre, 0); 
         Fadd32(fml,CLI_CODIGO_POSTAL,codigoPostal2, 0);
      }
  }/*Fin for direcciones;*/         
  }

   if (strcmp(rqst->name, "CliRecRegModTel") == 0)
   {
   for (i=0; i<lista->count;i++)
   {
      pRegMod = (tCliREGMOD *) QGetItem( lista, i);


      if (pRegMod->tipoModificacion == CLI_MODIFICACION_TELEFONO ||
          pRegMod->tipoModificacion == CLI_ELIMINACION_TELEFONO)
      {
         Fadd32(fml, CLI_CORRELATIVO, (char *)&pRegMod->correlativo, 0);
         Fadd32(fml, CLI_FECHA, pRegMod->fecha, 0);
         Fadd32(fml, CLI_USUARIO, (char *)&pRegMod->usuario, 0);
         Fadd32(fml, CLI_SUCURSAL,(char *)&pRegMod->sucursal, 0);
         Fadd32(fml, CLI_TIPO_MODIFICACION,(char *)&pRegMod->tipoModificacion, 0);
         Fadd32(fml, CLI_RUTX, (char *)&pRegMod->rutCliente, 0);
         
         identificador= pRegMod->usuario;
         if( identificador!= identificadorAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, HRM_TIPO, (char *)&PARCIAL_POR_IDENTIFICADOR, 0);
            Fadd32(fml2, HRM_IDENTIFICADOR_USUARIO, (char *)&identificador, 0);

            if (tpcall("HrmRecUsuarios", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(nombre, "S/A");
            else
                Fget32(fml2, HRM_NOMBRE, 0, nombre, 0);

             tpfree((char *)fml2);
            identificadorAnterior = identificador;
         }

         f_strcpylng(&rut,pRegMod->modificacion,9);
         f_strcpysht(&tipo , pRegMod->modificacion+9, 4);
         tipoTelefono.codigo = tipo;
         sts = SqlCliRecuperarTipoTelefono(&tipoTelefono);
         if (sts != SQL_SUCCESS)
         {
            strcpy(tipoTelefono.descripcion, "Tipo inexistente");
         }
         f_strcpystr (numero,pRegMod->modificacion+13, 20);
         numero[20] = '\0';
         strcpy(numero2, numero);
         Fadd32(fml,CLI_TIPO_TELEFONO, (char *)&tipoTelefono.codigo, 0);
         Fadd32(fml,CLI_DESCRIPCION_TELEFONO ,tipoTelefono.descripcion, 0);
         Fadd32(fml,CLI_NOMBRE,nombre, 0); 
         Fadd32(fml,CLI_NUMERO,numero2, 0);
      }
  }/*Fin for telefonos;*/         
  }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/*Objetivo: Validar un cliente por operacion y rut                      */
/*Autor: Cristian Osorio                                                */
/*Fecha:01-2007                                                         */
/*                                                                      */
/************************************************************************/
void CliValRutOpe(TPSVCINFO *rqst)
{
FBFR32         *fml;
   int          transaccionGlobal;
   long         sts;
   long         contador = 0;
   double       divisor = DIVISOR;
   short        codigoProducto;
   char         operacion[13+1];
   short        validar;
   tExcRutOpe  registroRutOpe;

/***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT,0,  (char *)&registroRutOpe.rutCliente, 0);
   Fget32(fml, CLI_OPERACION_FLUJO,0,  (char *)&registroRutOpe.operacion, 0);
   Fget32(fml, CLI_ACTIVIDAD,0, (char *)&validar, 0);


/******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

  if(!validar)
  {
  contador = 1;
  Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
  TRX_COMMIT(transaccionGlobal);
  tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
  }

   sprintf(operacion,"%0.0f",registroRutOpe.operacion);


  if ( strlen(operacion) == 11)
  {
  codigoProducto = (registroRutOpe.operacion) / (divisor);
  }
  else if ( strlen(operacion) == 12)
  {
  codigoProducto = (registroRutOpe.operacion) / (divisor);
  }
  else if ( strlen(operacion) == 13)
  {
  codigoProducto = (registroRutOpe.operacion) / (divisor * 10);
  }
  else
  {
  codigoProducto = 70;
  }
  
    switch(codigoProducto)
    {
    case CUENTA_CORRIENTE_PERSONA:
    case CUENTA_CORRIENTE_EMPRESA:
    case CUENTA_VISTA:
                       sts = SqlValidaCuentaCorriente(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaCuentaCorriente", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case LINEA_SOBREGIRO_PERSONA:
    case LINEA_SOBREGIRO_EMPRESA:
    case LINEA_SOBREGIRO_CUENTA_VISTA:
                         sts = SqlValidaLineaSobregiro(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaLineaSobregiro", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case CREDITO_MICRO_EMPRESA:
    case CREDITO_CONSUMO:
    case CREDITO_AUTOMOTRIZ:
    case REFINANCIAMIENTO:
    case RENEGOCIACION:
    case CONSOLIDACION_DEUDA:
    case CREDITO_BALLOON:
    case CREDITO_UNIVERSITARIO:
    case CREDITO_AMORTIZACION_DIFERIDA:
    case CONSOLIDACION_BALLOON:
    case UNIV_RECURSO_PROPIO:
                         sts = SqlValidaCredito(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "SqlValidaCredito", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case LINEA_CREDITO_CUOTA:
                         sts = SqlValidaLineaCreditoCuota(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaLineaCreditoCuota", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case LINEA_CREDITO_DISTRIBUIDOR:
                         sts = SqlValidaLineaCreditoDistribuidor(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaLineaCreditoDistribuidor", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case CREDITO_HIPOTECARIO:
                         sts = SqlValidaLineaCreditoHipotecario(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaLineaCreditoHipotecario", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case CUENTA_GASTO:
                         sts = SqlValidaCuentaGasto(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaCuentaGasto", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case DEPOSITO_PLAZO:
                        sts = SqlValidaDepositoPlazo(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaDepositoPlazo", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case AHORRO_PLAZO:
                        sts = SqlValidaAhorroPlazo(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaAhorroPlazo", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break;
    case TARJETA_CREDITO:
                        sts = SqlValidaTarjetaCredito(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaTarjetaCredito", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break; /* INI-MEJORAS CRUGE */
    case LINEA_CRUGE: 
                        sts = SqlValidaLineaCruge(&registroRutOpe,&contador);
                         if(sts  != SQL_SUCCESS)
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlValidaLineaCruge", 0);
                         TRX_ABORT(transaccionGlobal);
                         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
                         }
                         else
                         {
                         Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                         TRX_COMMIT(transaccionGlobal);
                         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                         }
                         break; /*FIN-MEJORAS CRUGE */
                 default:
                        Fadd32(fml , CLI_ESTADOCIVIL,   (char *)&contador    , 0 );
                        TRX_COMMIT(transaccionGlobal);
                        tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
                        break;
    }	

}


/************************************************************************/
/* Objetivo: Obtener tipo de Cliente                                    */
/* Autor   : Dinorah Painepan.            Fecha: Febrero 2007           */
/************************************************************************/
void CliRecBancaCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliBANCACLIENTE cliente;
   int totalRegistros=0, i=0;
 
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&cliente.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT, 0L) == -1)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, 
            "Recuperar Banca Cliente - ERROR: Fallo al iniciar transaccion.", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }
 
     sts = SqlCliRecuperarBancaCliente(&cliente);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente no existe.", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la información del cliente.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     Fadd32(fml , CLI_TIPOPERSONA,   (char *)&cliente.tipoCliente    , 0 );

     TRX_COMMIT(transaccionGlobal);
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/************************************************************************/
/* Objetivo: visar datos del cliente empresa                            */
/* Autor   : Dinorah Painepan.            Fecha: Febrero 2007           */
/************************************************************************/
void CliVisarCliEmp(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLIENTEEMPRESA cliente;
   int totalRegistros=0, i=0;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&cliente.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT, 0L) == -1)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, 
            "Recuperar Banca Cliente - ERROR: Fallo al iniciar transaccion.", 0);
        tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
     }
     sts = SqlCliRecuperarClienteEmpresa(&cliente);
 
     if (sts != SQL_SUCCESS)
     {
       if (sts == SQL_NOT_FOUND)
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente no existe.", 0);
       else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la información del cliente.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     if (cliente.estado != CLI_ESTADO_NO_VISADO)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Cliente en estado inadecuado", 0);
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
     sts = SqlCliVisarClienteEmpresa(&cliente);
     if (sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Problemas al actualizar datos de visación.", 0);
        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }
     Fadd32(fml , CLI_FECHAVISACION,   cliente.fechaVisacion    , 0 );
     Fadd32(fml , CLI_FECHAVIGENCIA,   cliente.fechaVigencia    , 0 );

     TRX_COMMIT(transaccionGlobal);
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Insertar apoderados de un cliente empresa                  */
/* Autor   : Dinorah Painepan.                      Fecha: Febrero 2007 */
/************************************************************************/
void CliIngElmModApo(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   accion;
   int    transaccionGlobal;
   tCliAPODERADO  apoderado;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   sts  = Fget32(fml, CLI_CODIGO_ACCION, 0, (char *)&accion, 0);
   sts |= Fget32(fml, CLI_RUT_EMPRESA, 0, (char *)&apoderado.rut, 0);
   sts |= Fget32(fml, CLI_RUT, 0, (char *)&apoderado.rutCliente, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (accion != CLI_ACCION_ELIMINAR)
   {
      sts |= Fget32(fml, CLI_FLAG_SOCIO, 0, apoderado.esSocio, 0);
      sts |= Fget32(fml, CLI_FLAG_REPRESENTANTE, 0, apoderado.esRepresentante, 0);
      sts |= Fget32(fml, CLI_PORCENTAJE_PARTICIPACION, 0, (char *)&apoderado.porcentajeParticipacion, 0);
   }
   if (sts == -1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar datos iniciales", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   switch (accion)
   {
     case CLI_ACCION_INSERTAR: sts = SqlCliInsertarApoderado(&apoderado);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear apoderado", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_MODIFICAR: sts = SqlCliModificarApoderado(&apoderado);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar apoderado", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_ELIMINAR: sts = SqlCliEliminarApoderado(&apoderado);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar apoderado", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/************************************************************************/
/* Objetivo: Recuperar apoderados de un cliente empresa                 */
/* Autor   : Dinorah Painepan.                      Fecha: Marzo 2007   */
/************************************************************************/
void CliRecApoderado(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long     sts;
   tCliAPODERADO  *tApoderado;
   tCliAPODERADO  apoderado;
   Q_HANDLE *lista;
   long     rut, rutCliente;
   char     opcionRecuperacion[2];
   int transaccionGlobal, i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   sts = Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);
   sts = Fget32(fml, CLI_RUT_EMPRESA, 0, (char *)&rut, 0);
   if (strcmp (opcionRecuperacion,"T") != 0)
       sts = Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp (opcionRecuperacion,"T") == 0)
   {
      if ((lista = QNew_(10,10)) == NULL)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
      sts = SqlCliRecuperarApoderadoRut(lista, rut);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      } 
      fml = NewFml32((long)lista->count, (short)1, (short)5, (short)2, (long)2);
      if (fml == NULL)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
      }
      for (i = 0; i < lista->count ; i++)
      {
         tApoderado = QGetItem(lista, i);
         Fadd32(fml, CLI_RUT_EMPRESA, (char *)&tApoderado->rut, 0);
         Fadd32(fml, CLI_RUT, (char *)&tApoderado->rutCliente, 0);
         Fadd32(fml, CLI_FLAG_SOCIO, tApoderado->esSocio, 0);
         Fadd32(fml, CLI_FLAG_REPRESENTANTE, tApoderado->esRepresentante, 0);
         Fadd32(fml, CLI_PORCENTAJE_PARTICIPACION, (char *)&tApoderado->porcentajeParticipacion, 0);
      }
      QDelete(lista);
   }
   else
   {
      apoderado.rut = rut;
      apoderado.rutCliente = rutCliente;
      sts = SqlCliRecuperarApoderadoRutUno(&apoderado);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registro", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
      Fadd32(fml, CLI_RUT_EMPRESA, (char *)&apoderado.rut, 0);
      Fadd32(fml, CLI_RUT, (char *)&apoderado.rutCliente, 0);
      Fadd32(fml, CLI_FLAG_SOCIO, apoderado.esSocio, 0);
      Fadd32(fml, CLI_FLAG_REPRESENTANTE, apoderado.esRepresentante, 0);
      Fadd32(fml, CLI_PORCENTAJE_PARTICIPACION, (char *)&apoderado.porcentajeParticipacion, 0);
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/************************************************************************/
/* Objetivo: Insertar informe legal de cliente empresa                  */
/* Autor   : Dinorah Painepan.                      Fecha: Marzo 2007   */
/************************************************************************/
void CliIngElmModILe(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   accion;
   int    transaccionGlobal;
   tCliINFORMELEGAL  informeLegal;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   sts  = Fget32(fml, CLI_CODIGO_ACCION, 0, (char *)&accion, 0);
   sts |= Fget32(fml, CLI_RUT, 0, (char *)&informeLegal.rut, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);
   if (accion != CLI_ACCION_ELIMINAR)
   {
      sts |= Fget32(fml, CLI_FECHA_CONSTITUCION, 0,informeLegal.fechaConstitucion, 0);
      sts |= Fget32(fml, CLI_NOTARIA_CONSTITUCION, 0,informeLegal.notariaConstitucion, 0);
      sts |= Fget32(fml, CLI_CIUDAD_CONSTITUCION, 0,(char *)&informeLegal.ciudadConstitucion, 0);
      sts |= Fget32(fml, CLI_FOJAS_INSCRIPCION, 0,informeLegal.fojasInscripcion, 0);
      sts |= Fget32(fml, CLI_NUMERO_INSCRIPCION, 0,(char *)&informeLegal.numeroInscripcion, 0);
      sts |= Fget32(fml, CLI_FECHA_INSCRIPCION, 0,informeLegal.fechaInscripcion, 0);
      sts |= Fget32(fml, CLI_REGISTRO_COMERCIO, 0,informeLegal.registroComercio, 0);
      sts |= Fget32(fml, CLI_DIARIO_PUBLICACION, 0,informeLegal.diarioPublicacion, 0);
      sts |= Fget32(fml, CLI_FECHA_PUBLICACION, 0,informeLegal.fechaPublicacion, 0);
      sts |= Fget32(fml, CLI_CIUDAD_LEGAL,      0,(char *)&informeLegal.ciudadLegal, 0);
      sts |= Fget32(fml, CLI_DURACION,          0,(char *)&informeLegal.duracion, 0);
      sts |= Fget32(fml, CLI_OBJETO, 0,informeLegal.objeto, 0);
      sts |= Fget32(fml, CLI_NUMERO_MIEMBRO,    0,(char *)&informeLegal.numeroMiembroSA, 0);
      sts |= Fget32(fml, CLI_DURACION_CARGO,    0,(char *)&informeLegal.duracionCargoSA, 0);
      sts |= Fget32(fml, CLI_MONTO_CAPITAL, 0,(char *)&informeLegal.montoCapital, 0);
      sts |= Fget32(fml, CLI_MONEDA_CAPITAL, 0,(char *)&informeLegal.monedaCapital, 0);
      sts |= Fget32(fml, CLI_CONCLUSION, 0,informeLegal.conclusion, 0);
      sts |= Fget32(fml, CLI_ADMINISTRACION, 0,informeLegal.administracion, 0);
   }
   if (sts == -1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar datos iniciales", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   switch (accion)
   {
     case CLI_ACCION_INSERTAR: sts = SqlCliInsertarInformeLegal(&informeLegal);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear informe legal", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_MODIFICAR: sts = SqlCliModificarInformeLegal(&informeLegal);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar informe legal", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_ELIMINAR: sts = SqlCliEliminarInformeLegal(&informeLegal);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar informe legal", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/************************************************************************/
/* Objetivo: Recuperar informe legal de un cliente empresa              */
/* Autor   : Dinorah Painepan.                      Fecha: Marzo 2007   */
/************************************************************************/
void CliRecInfLegal(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long     sts;
   tCliINFORMELEGAL  informeLegal;
   int transaccionGlobal, i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   sts = Fget32(fml, CLI_RUT, 0, (char *)&informeLegal.rut, 0);
   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarInformeLegal(&informeLegal);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar informe legal", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   } 
   fml = NewFml32((long)1, (short)4, (short)8, (short)10, (long)1000);
   if (fml == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }
   Fadd32(fml, CLI_RUT,                (char *)&informeLegal.rut, 0);
   Fadd32(fml, CLI_FECHA_CONSTITUCION, informeLegal.fechaConstitucion, 0);
   Fadd32(fml, CLI_NOTARIA_CONSTITUCION, informeLegal.notariaConstitucion, 0);
   Fadd32(fml, CLI_CIUDAD_CONSTITUCION, (char *)&informeLegal.ciudadConstitucion, 0);
   Fadd32(fml, CLI_FOJAS_INSCRIPCION, informeLegal.fojasInscripcion, 0);
   Fadd32(fml, CLI_NUMERO_INSCRIPCION, (char *)&informeLegal.numeroInscripcion, 0);
   Fadd32(fml, CLI_FECHA_INSCRIPCION, informeLegal.fechaInscripcion, 0);
   Fadd32(fml, CLI_REGISTRO_COMERCIO, informeLegal.registroComercio, 0);
   Fadd32(fml, CLI_DIARIO_PUBLICACION, informeLegal.diarioPublicacion, 0);
   Fadd32(fml, CLI_FECHA_PUBLICACION, informeLegal.fechaPublicacion, 0);
   Fadd32(fml, CLI_CIUDAD_LEGAL,      (char *)&informeLegal.ciudadLegal, 0);
   Fadd32(fml, CLI_DURACION,          (char *)&informeLegal.duracion, 0);
   Fadd32(fml, CLI_OBJETO, informeLegal.objeto, 0);
   Fadd32(fml, CLI_NUMERO_MIEMBRO,    (char *)&informeLegal.numeroMiembroSA, 0);
   Fadd32(fml, CLI_DURACION_CARGO,    (char *)&informeLegal.duracionCargoSA, 0);
   Fadd32(fml, CLI_MONTO_CAPITAL, (char *)&informeLegal.montoCapital, 0);
   Fadd32(fml, CLI_MONEDA_CAPITAL, (char *)&informeLegal.monedaCapital, 0);
   Fadd32(fml, CLI_CONCLUSION, informeLegal.conclusion, 0);
   Fadd32(fml, CLI_ADMINISTRACION, informeLegal.administracion, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Mantener nombre fantasía de un cliente empresa             */
/* Autor   : Dinorah Painepan.                       Fecha: Marzo 2007  */
/************************************************************************/
void CliInsModElmFan(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long   sts;
   long   accion;
   int    transaccionGlobal;
   tCliNOMBREFANTASIA fantasia;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   sts  = Fget32(fml, CLI_CODIGO_ACCION, 0, (char *)&accion, 0);
   sts |= Fget32(fml, CLI_RUT, 0, (char *)&fantasia.rut, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   switch (accion)
   {
      case CLI_ACCION_INSERTAR  : sts |= Fget32(fml, CLI_NOMBRE_FANTASIA_EMPRESA, 0, fantasia.nombre, 0);
                                  break;
      case CLI_ACCION_MODIFICAR : sts |= Fget32(fml, CLI_NOMBRE_FANTASIA_EMPRESA, 0, fantasia.nombre, 0);
                                  sts |= Fget32(fml, CLI_CORRELATIVO, 0, (char *)&fantasia.correlativo, 0);
                                  break;
      case CLI_ACCION_ELIMINAR  : sts |= Fget32(fml, CLI_CORRELATIVO, 0, (char *)&fantasia.correlativo, 0);
                                  break;
   }
   if (sts == -1)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar datos iniciales", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   switch (accion)
   {
     case CLI_ACCION_INSERTAR: sts = SqlCliCrearNombreFantasiaEmpresa(&fantasia);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear nombre de fantasía", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_MODIFICAR: sts = SqlCliModificarNombreFantasiaEmpresa(&fantasia);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar nombre de fantasía", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
     case CLI_ACCION_ELIMINAR: sts = SqlCliEliminarNombreFantasiaEmpresa(&fantasia);
                    if (sts != SQL_SUCCESS)
                    {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar nombre de fantasía", 0);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                    }
                    break;
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/************************************************************************/
/* Objetivo: Recuperar nombres de fantasía de un cliente empresa        */
/* Autor   : Dinorah Painepan.                      Fecha: Marzo 2007   */
/************************************************************************/
void CliRecNombreFan(TPSVCINFO *rqst)
{
   FBFR32   *fml;
   long     sts;
   tCliNOMBREFANTASIA  *tFantasia;
   tCliNOMBREFANTASIA  fantasia;
   Q_HANDLE *lista;
   long     rut;
   long     correlativo;
   char     opcionRecuperacion[2];
   int transaccionGlobal, i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   sts = Fget32(fml, CLI_OPCION_RECUPERACION, 0, opcionRecuperacion, 0);
   sts = Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
   if (strcmp (opcionRecuperacion,"T") != 0)
       sts = Fget32(fml, CLI_CORRELATIVO, 0, (char *)&correlativo, 0);

   /******   Cuerpo del servicio   ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if (strcmp (opcionRecuperacion,"T") == 0)
   {
      if ((lista = QNew_(10,10)) == NULL)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
      sts = SqlCliRecuperarNombreFantasiaRut(lista, rut);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
      fml = NewFml32((long)lista->count, (short)1, (short)1, (short)1, (long)80);
      if (fml == NULL)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
      }
      for (i = 0; i < lista->count ; i++)
      {
         tFantasia = QGetItem(lista, i);
         Fadd32(fml, CLI_RUT, (char *)&tFantasia->rut, 0);
         Fadd32(fml, CLI_CORRELATIVO, (char *)&tFantasia->correlativo, 0);
         Fadd32(fml, CLI_NOMBRE_FANTASIA_EMPRESA, tFantasia->nombre, 0);
         }
      QDelete(lista);
   }
   else
   {
      fantasia.rut = rut;
      fantasia.correlativo = correlativo;
      sts = SqlCliRecuperarNombreFantasia(&fantasia);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registro", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
      Fadd32(fml, CLI_RUT, (char *)&fantasia.rut, 0);
      Fadd32(fml, CLI_CORRELATIVO, (char *)&fantasia.correlativo, 0);
      Fadd32(fml, CLI_NOMBRE_FANTASIA_EMPRESA, fantasia.nombre, 0);
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar Descripcion del Tipo de Direccion                */
/* Autor   : Cristian Osorio                          Fecha: 20-03-2007 */
/* Modifica: Cristian Osorio                          Fecha: 20-03-2007 */
/************************************************************************/


void CliDescDire(TPSVCINFO *rqst)
{
 FBFR32         *fml;
   int          transaccionGlobal;
   long         sts;
   tCliCODIGODESCRIPCION   tipoDireccion;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO,0,  (char *)&tipoDireccion.codigo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);


   sts = SqlCliRecuperarTipoDireccion(&tipoDireccion);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe un tipo de direccion para este codigo ", 0);
      else
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "error ", 0);
      userlog("%s: Error en funcion SqlCliRecuperarTipoDireccion.", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   else
   {
   Fadd32(fml, CLI_DESCRIPCION,tipoDireccion.descripcion,           0);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
   }
}

/************************************************************************/
/* Objetivo: Recupera Flujo de Informacion con el boletin comercial     */
/* Autor   : Cristian Osorio                          Fecha: 28-03-2007 */
/* Modifica: Cristian Osorio                          Fecha: 28-03-2007 */
/************************************************************************/


  void CliRecFluCom(TPSVCINFO *rqst)
{
   FBFR32                       *fml;
   int                          transaccionGlobal;
   long                         sts;
   Q_HANDLE                     *listaFlujo;
   int                          i=0;
   tCliFLUJOINFORMACIONBOLETIN  *flujo;
   long                         rutCliente;

   fml = (FBFR32 *)rqst->data;

   Fget32(fml,  CLI_RUT,           0 , (char *)&rutCliente , 0 );

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((listaFlujo = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, " Error en QNew", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlRecuperarFlujo(listaFlujo, rutCliente);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registros de flujo de informacion para este cliente", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlProRecuperarFlujo", 0);
      userlog("%s: Error en funcion SqlProRecuperarFlujo.", rqst->name);
      QDelete(listaFlujo);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)listaFlujo->count, (short)1, (short)2, (short)5, (long)8);

   if (fml == NULL)
   {
        fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al asignar  NewFml32", 0);
        userlog("%s: Error en funcion NewFml32.", rqst->name);
        QDelete(listaFlujo);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < listaFlujo->count; i++)
   {
      flujo = (tCliFLUJOINFORMACIONBOLETIN *) QGetItem(listaFlujo, i);

      Fadd32(fml, CLI_OPERACION_FLUJO,                     (char *)&flujo->operacion            , i );
      Fadd32(fml, CLI_TIPO_CREDITO_FLUJO,                   flujo->tipoCredito                  , i );
      Fadd32(fml, CLI_MONTO_INFORMADO_FLUJO,               (char *)&flujo->montoInformado       , i );
      Fadd32(fml, CLI_MONEDA_FLUJO,                        (char *)&flujo->moneda               , i );
      Fadd32(fml, CLI_FECHA_VENCIMIENTO_FLUJO,              flujo->fechaVencimiento             , i );
      Fadd32(fml, CLI_FECHA_ENVIO_BOLETIN,                  flujo->fechaEnvioBoletinComercial   , i );
      Fadd32(fml, CLI_FECHA_PAGO_CUOTA,                     flujo->fechaPagoCuota               , i );
      Fadd32(fml, CLI_FECHA_ENVIO_PAGO,                     flujo->fechaEnvioPago               , i );
   }

   QDelete(listaFlujo);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/************************************************************************/
/*Objetivo: Mostrar el Historico de direcciones y telefonos con Distinct*/
/*Autor: Cristian Osorio                                                */
/*Fecha:03-2007                                                         */
/*                                                                      */
/************************************************************************/


  void CliRecRegDiDi(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   FBFR32 *fml2;
   short f=0;
   int j=0;
   int i=0;
   int transaccionGlobal=0;
   long sts=0;
   long  largo;
   Q_HANDLE *lista;
   tCliREGMOD *pRegMod;
   tCliREGMOD  regMod;
   tCliREGMOD  regMod1;
   tCliDIRECCION direccion;
   tCliTELEFONO telefono;
   tCliCODIGODESCRIPCION tipoTelefono;
   tCliCODIGODESCRIPCION tipoDeDireccion;
   long rut;
   short tipo;
   char numero[20 +1];
   char numero2[20 +1];
   char codarea[2 +1]; /* ACD */

   short tipoDireccion;
   char direccionCalleNumero[40+1];
   char direccionCalleNumero2[40+1];
   char departamento[8+1];
   char departamento2[8+1];
   char villaPoblacion[40+1];
   char villaPoblacion2[40 +1];
   char nombre[40 +1];
   short ciudad;
   short comuna;
   char codigoPostal[10+1];
   char codigoPostal2[10+1];
   long identificador;
   long identificadorAnterior;
   short ciudadAnterior=-1;
   short comunaAnterior=-1;
   short tipoDireccionAnterior=-1;
   char  ciudadDescripcion[50 + 1];
   char  comunaDescripcion[50 + 1];
   ciudadDescripcion[0] = '\0';
   short  PARCIAL_POR_IDENTIFICADOR=11;
   memset((void *)&regMod,0,sizeof(tCliREGMOD));
   memset((void *)&regMod1,0,sizeof(tCliREGMOD));

   fml = (FBFR32 *)rqst->data;

   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUTX, 0, (char *)&regMod.rutCliente, 0);

   if (Foccur32(fml, CLI_CORRELATIVO) > 0)
   {
      Fget32(fml, CLI_CORRELATIVO, 0, (char *)&regMod.correlativo, 0);
   }
   else
   {   regMod1.rutCliente= regMod.rutCliente;
       sts = SqlCliMAXCorrelativo(&regMod1);
       if (sts != SQL_SUCCESS)
       {
         if (sts != SQL_NOT_FOUND)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar correlativo.", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
      }
       regMod.correlativo= regMod1.correlativo;
   }

   if ((lista = QNew()) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear lista registro Modificacion.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarRegistroModificacionTodosDistinct(&regMod, lista);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registro Modificacion", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registro Modificacion", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   tpfree((char *)fml);
   fml = NewFml32((long)lista->count, (short)6, (short)4, (short)4, (long)248);
   if (fml == NULL)
   {
      userlog("%s No es posible realizar la accion solicitada. (NewFml32).", rqst->name);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "No es posible realizar la accion solicitada", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (strcmp(rqst->name, "CliRecRegDirDi") == 0)
   {
   for (i=0; i<lista->count;i++)
   {
      pRegMod = (tCliREGMOD *) QGetItem( lista, i);

      if (pRegMod->tipoModificacion == CLI_MODIFICACION_DIRECCION ||
          pRegMod->tipoModificacion == CLI_ELIMINACION_DIRECCION)
      {

         Fadd32(fml, CLI_FECHA, pRegMod->fecha, 0);
         Fadd32(fml, CLI_USUARIO, (char *)&pRegMod->usuario, 0);
         Fadd32(fml, CLI_SUCURSAL,(char *)&pRegMod->sucursal, 0);
         Fadd32(fml, CLI_TIPO_MODIFICACION,(char *)&pRegMod->tipoModificacion, 0);
         Fadd32(fml, CLI_RUTX, (char *)&pRegMod->rutCliente, 0);
         identificador= pRegMod->usuario;
         if( identificador!= identificadorAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, HRM_TIPO, (char *)&PARCIAL_POR_IDENTIFICADOR, 0);
            Fadd32(fml2, HRM_IDENTIFICADOR_USUARIO, (char *)&identificador, 0);

            if (tpcall("HrmRecUsuarios", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(nombre, "S/A");
            else
                Fget32(fml2, HRM_NOMBRE, 0, nombre, 0);

             tpfree((char *)fml2);
            identificadorAnterior = identificador;
         }

         f_strcpylng(&rut,pRegMod->modificacion,9);
         f_strcpysht(&tipoDireccion ,pRegMod->modificacion+9, 4);
         direccion.rutCliente= rut;
         direccion.tipoDireccion= tipoDireccion;

         if(tipoDireccion != tipoDireccionAnterior)
         {
            tipoDeDireccion.codigo = tipoDireccion;
            sts = SqlCliRecuperarTipoDireccion(&tipoDeDireccion);
            if (sts != SQL_SUCCESS)
            {
               strcpy(tipoDeDireccion.descripcion, "Tipo inexistente");
            }
            tipoDireccionAnterior = tipoDireccion;
         }

         f_strcpystr(direccionCalleNumero,pRegMod->modificacion+13, 40);
         direccionCalleNumero[40] = '\0';
         strcpy(direccionCalleNumero2, direccionCalleNumero);

         f_strcpystr(departamento,pRegMod->modificacion+53, 8);
         departamento[8] = '\0';
         strcpy(departamento2, departamento);

         f_strcpystr(villaPoblacion,pRegMod->modificacion+61, 40);
         villaPoblacion[40] = '\0';
         strcpy(villaPoblacion2, villaPoblacion);

         f_strcpysht(&ciudad , pRegMod->modificacion+101, 4);
         f_strcpysht(&comuna , pRegMod->modificacion+105, 4);

         f_strcpystr(codigoPostal,pRegMod->modificacion+109, 10);
         codigoPostal[10] = '\0';
         strcpy(codigoPostal2, codigoPostal);

         if(ciudad != ciudadAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, SGT_CODIGO, (char *)&ciudad, 0);

            if (tpcall("SgtRecCiudad", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(ciudadDescripcion, "Ciudad inexistente");
            else
               Fget32(fml2, SGT_DESCRIPCION, 0, ciudadDescripcion, 0);
            tpfree((char *)fml2);
            ciudadAnterior = ciudad;
         }

         if(comuna != comunaAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, SGT_CODIGO, (char *)&comuna, 0);

            if (tpcall("SgtRecComuna", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(comunaDescripcion, "Comuna inexistente");
            else
               Fget32(fml2, SGT_DESCRIPCION, 0, comunaDescripcion, 0);
            tpfree((char *)fml2);
            comunaAnterior = comuna;
         }

         Fadd32(fml,CLI_TIPO_DIRECCION,(char *)&tipoDeDireccion.codigo, 0);
         Fadd32(fml,CLI_DIRECCION,tipoDeDireccion.descripcion, 0);
         Fadd32(fml,CLI_CALLE_NUMERO,direccionCalleNumero2, 0);
         Fadd32(fml,CLI_DEPARTAMENTO,departamento2, 0);
         Fadd32(fml,CLI_VILLA_POBLACION,villaPoblacion2, 0);
         Fadd32(fml,CLI_CIUDAD,(char *)&ciudad, 0);
         Fadd32(fml,CLI_DESCRIPCION_CIUDAD,ciudadDescripcion, 0);
         Fadd32(fml,CLI_DESCRIPCION_COMUNA,comunaDescripcion, 0);
         Fadd32(fml,CLI_NOMBRE,nombre, 0);
         Fadd32(fml,CLI_CODIGO_POSTAL,codigoPostal2, 0);
      }
  }
  }


   if (strcmp(rqst->name, "CliRecRegTelDi") == 0)
   {
   for (i=0; i<lista->count;i++)
   {
      pRegMod = (tCliREGMOD *) QGetItem( lista, i);


      if (pRegMod->tipoModificacion == CLI_MODIFICACION_TELEFONO ||
          pRegMod->tipoModificacion == CLI_ELIMINACION_TELEFONO)
      {
         Fadd32(fml, CLI_FECHA, pRegMod->fecha, 0);
         Fadd32(fml, CLI_USUARIO, (char *)&pRegMod->usuario, 0);
         Fadd32(fml, CLI_SUCURSAL,(char *)&pRegMod->sucursal, 0);
         Fadd32(fml, CLI_TIPO_MODIFICACION,(char *)&pRegMod->tipoModificacion, 0);
         Fadd32(fml, CLI_RUTX, (char *)&pRegMod->rutCliente, 0);

         identificador= pRegMod->usuario;
         if( identificador!= identificadorAnterior)
         {
            fml2 = (FBFR32 *)tpalloc("FML32",NULL, 1024);
            Fadd32(fml2, HRM_TIPO, (char *)&PARCIAL_POR_IDENTIFICADOR, 0);
            Fadd32(fml2, HRM_IDENTIFICADOR_USUARIO, (char *)&identificador, 0);

            if (tpcall("HrmRecUsuarios", (char *)fml2, 0L, (char **)&fml2, &largo, TPNOTRAN) == -1)
               strcpy(nombre, "S/A");
            else
                Fget32(fml2, HRM_NOMBRE, 0, nombre, 0);

             tpfree((char *)fml2);
            identificadorAnterior = identificador;
         }

         f_strcpylng(&rut,pRegMod->modificacion,9);
         f_strcpysht(&tipo , pRegMod->modificacion+9, 4);
         tipoTelefono.codigo = tipo;
         sts = SqlCliRecuperarTipoTelefono(&tipoTelefono);
         if (sts != SQL_SUCCESS)
         {
            strcpy(tipoTelefono.descripcion, "Tipo inexistente");
         }
         f_strcpystr (numero,pRegMod->modificacion+13, 20);
         numero[20] = '\0';

         /* ACD */
         f_strcpystr (codarea,pRegMod->modificacion+13+20, 2);
         codarea[2] = '\0';

         sprintf(numero2,"%02s %20s", codarea,numero);
         /* ACD */

         Fadd32(fml,CLI_TIPO_TELEFONO, (char *)&tipoTelefono.codigo, 0);
         Fadd32(fml,CLI_DESCRIPCION_TELEFONO ,tipoTelefono.descripcion, 0);
         Fadd32(fml,CLI_NOMBRE,nombre, 0);
         Fadd32(fml,CLI_NUMERO,numero2, 0);
      }
  }
  }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/*Objetivo: Verificar si un cliente posee fotografía, huella o firma    */
/*Autor: Cristian Osorio                                                */
/*Fecha:04-2007                                                         */
/*                                                                      */
/************************************************************************/

void CliVerDatos(TPSVCINFO *rqst)
{
	FBFR32		*fml;
	int		transaccionGlobal=0;
	long		sts1=0;
	long		sts2=0;
	long		sts3=0;
	long		sts4=0;
	long		contador1=0;
	long		contador2=0;
	long		contador3=0;
	long		contador4=0;
	tCliCLIENTE  cliente;

	memset((void *)&cliente,0,sizeof(tCliCLIENTE));

	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml,  CLI_RUT,               0 , (char *)&cliente.rut     , 0 );
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts1 = SqlCuentaRegistroFotografia(&cliente,&contador1);
	
	if(sts1 != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCuentaRegistroFotografia", 0);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts1, rqst->name), (char *)fml, 0L, 0L);
	}

	sts2 = SqlCuentaRegistroHuella(&cliente,&contador2);
	
	if(sts2 != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCuentaRegistroHuella", 0);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts2, rqst->name), (char *)fml, 0L, 0L);
	}
	
	sts3 = SqlCuentaRegistroFirma(&cliente,&contador3);
	
	if(sts3 != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCuentaRegistroFirma", 0);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts3, rqst->name), (char *)fml, 0L, 0L);
	}
	
	sts4 = SqlCuentaRegistroSegmentacion(&cliente,&contador4);
	
	if(sts4 != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCuentaRegistroSegmentacion", 0);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts4, rqst->name), (char *)fml, 0L, 0L);
	}
	
	Fadd32(fml , CLI_EXISTE_FOTOGRAFIA,   (char *)&contador1    , 0 );
	Fadd32(fml , CLI_EXISTE_HUELLA,       (char *)&contador2    , 0 );
	Fadd32(fml , CLI_EXISTE_FIRMA,        (char *)&contador3    , 0 );
	Fadd32(fml , CLI_EXISTE_SEG,          (char *)&contador4    , 0 );
	
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecMatSeg
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    15-11-2007
  *  DESCRIPTION
  *    Recuperar Codigo de Segmento comercial y categoria ocupacional de un cliente
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecMatSeg(TPSVCINFO *rqst)
{
	FBFR32	*fml;
	int	transaccionGlobal=0;
	long	sts=0;
	long	rut=0;
	short	codigoSegmentoComercial=0;
	short	codigoCategoriaOcupacional=0;
	
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_RUT,0,  (char *)&rut, 0);
	
	/******   Cuerpo del servicio   ******/
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts = SqlRecuperarCodigoSegmentoCategoria(rut,&codigoSegmentoComercial,&codigoCategoriaOcupacional);
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Este rut no esta segmentado", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "error en consulta SqlRecuperarCodigoSegmentoCategoria", 0);
			userlog("%s: Error en funcion SqlRecuperarCodigoSegmentoCategoria.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	else
	{
		Fadd32(fml, CLI_COD_SEG_COM,(char *)&codigoSegmentoComercial,           0);
		Fadd32(fml, CLI_COD_OCUPA,(char *)&codigoCategoriaOcupacional,           0);
		TRX_COMMIT(transaccionGlobal);
		tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
	}
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecSegCom
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    16-11-2007
  *  DESCRIPTION
  *    Recuperar Datos de la tabla SegmentoComercial
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecSegCom(TPSVCINFO *rqst)
{
	FBFR32				*fml;
	int				transaccionGlobal=0;
	long				sts=0;
	tCliSegmentacionComercial	segmentacionComercial;
	
	memset((void *)&segmentacionComercial,0,sizeof(tCliSegmentacionComercial));
	
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_COD_SEG_COM,0,  (char *)&segmentacionComercial.codigo, 0);
	
	/******   Cuerpo del servicio   ******/
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts = SqlCliRecuperarDatosSegmentacion(&segmentacionComercial);
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe datos de segmentacion con ese codigo ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "error ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarDatosSegmentacion.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	else
	{
		Fadd32(fml, CLI_SEG_DESCRIPCION,segmentacionComercial.descripcion,      0);
		Fadd32(fml, CLI_SEG_CARACT,segmentacionComercial.caracteristica,        0);
		Fadd32(fml, CLI_SEG_CANAL,segmentacionComercial.canal,                  0);
		Fadd32(fml, CLI_SEG_PROMO,segmentacionComercial.promocion,              0);
		TRX_COMMIT(transaccionGlobal);
		tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
	}
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecOferSeg
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    16-11-2007
  *  DESCRIPTION
  *    Recuperar Ofertas de un Segmento
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecOferSeg(TPSVCINFO *rqst)
{
	FBFR32			*fml;
	int			transaccionGlobal=0;
	long			sts=0;
	Q_HANDLE		*listaOfertas;
	int			i=0;
	short			codigoSegmentoComercial=0;
	tCliOfertasSegmento	*ofertasSegmento;
	
        ofertasSegmento = (tCliOfertasSegmento *)NULL;
	
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_COD_SEG_COM,0,  (char *)&codigoSegmentoComercial, 0);
	
	/******   Cuerpo del servicio   ******/
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	if ((listaOfertas = QNew_(10,10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecOfertasSegmento(codigoSegmentoComercial,listaOfertas);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen caracteristicas para este segmento", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecOfertasSegmento", 0);
			userlog("%s: Error en SqlCliRecOfertasSegmento.", rqst->name);
			QDelete(listaOfertas);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	fml = NewFml32((long)listaOfertas->count, (short)1, (short)0, (short)1, (long)300);
	
	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaOfertas);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY,rqst->name), (char *)fml, 0L, 0L);
	}
	
	for (i=0; i < listaOfertas->count; i++)
	{
		ofertasSegmento = (tCliOfertasSegmento *) QGetItem(listaOfertas, i);
		
		Fadd32(fml, CLI_OFE_SEG,                (char *)&ofertasSegmento->codigo        ,       i);
		Fadd32(fml, CLI_DESCRIPCION_OFE,        ofertasSegmento->descripcion            ,       i);
	}
	
	QDelete(listaOfertas );
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecProSeg
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    19-11-2007
  *  DESCRIPTION
  *    Recuperar Productos Financieros de un Segmento
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecProSeg(TPSVCINFO *rqst)
{
	FBFR32			*fml;
	int			transaccionGlobal=0;
	long			sts=0;
	Q_HANDLE		*listaProductos;
	int			i=0;
	short			codigoSegmentoComercial=0;
	tCliOfertasSegmento	*productosSegmento;
	
        productosSegmento = (tCliOfertasSegmento *)NULL;
	
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_COD_SEG_COM,0,  (char *)&codigoSegmentoComercial, 0);
	
	/******   Cuerpo del servicio   ******/
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	if ((listaProductos = QNew_(10,10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecProductosSegmento(codigoSegmentoComercial,listaProductos);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen productos para este segmento", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecProductosSegmento", 0);
			userlog("%s: Error en SqlCliRecProductosSegmento.", rqst->name);
			QDelete(listaProductos);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	fml = NewFml32((long)listaProductos->count, (short)1, (short)0, (short)0, (long)300);
	
	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaProductos);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY,rqst->name), (char *)fml, 0L, 0L);
	}
	
	for (i=0; i < listaProductos->count; i++)
	{
		productosSegmento = (tCliOfertasSegmento *) QGetItem(listaProductos, i);
		
		Fadd32(fml, CLI_PROD_SEG,                (char *)&productosSegmento->codigo,       i);
		Fadd32(fml, CLI_DESCRIPCION_PRO,         productosSegmento->descripcion,           i);
	}
	
	QDelete(listaProductos );
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecNecSeg
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    19-11-2007
  *  DESCRIPTION
  *    Recuperar Necesidades Financieras de un Segmento
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecNecSeg(TPSVCINFO *rqst)
{
	FBFR32			*fml;
	int			transaccionGlobal=0;
	long			sts=0;
	Q_HANDLE		*listaNecesidades;
	int			i=0;
	tCliOfertasSegmento	*necesidadesSegmento;
	short			codigoSegmentoComercial=0;
	
        necesidadesSegmento = (tCliOfertasSegmento *)NULL;

	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_COD_SEG_COM,0,  (char *)&codigoSegmentoComercial, 0);
	
	/******   Cuerpo del servicio   ******/
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	if ((listaNecesidades = QNew_(10,10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecNecesidadesSegmento(codigoSegmentoComercial,listaNecesidades);
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen Necesidades Financieras para este segmento", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecNecesidadesSegmento", 0);
			userlog("%s: Error en SqlCliRecNecesidadesSegmento.", rqst->name);
			QDelete(listaNecesidades);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	fml = NewFml32((long)listaNecesidades->count, (short)1, (short)0, (short)0, (long)300);
	
	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaNecesidades);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY,rqst->name), (char *)fml, 0L, 0L);
	}
	
	for (i=0; i < listaNecesidades->count; i++)
	{
		necesidadesSegmento = (tCliOfertasSegmento *) QGetItem(listaNecesidades, i);
		
		Fadd32(fml, CLI_NEC_SEG,                (char *)&necesidadesSegmento->codigo,       i);
		Fadd32(fml, CLI_DESCRIPCION_NEC,         necesidadesSegmento->descripcion,           i);
	}
	
	QDelete(listaNecesidades );
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecCatOcu
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    20-11-2007
  *  DESCRIPTION
  *    Recuperar Todos los datos de la tabla Categoria Ocupacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecCatOcu(TPSVCINFO *rqst)
{
	FBFR32				*fml;
	int				transaccionGlobal=0;
	long				sts=0;
	Q_HANDLE			*listaCategoriaOcupacional;
	int				i=0;
	tCliCategoriaOcupacional	*categoriaOcupacional;
	
	categoriaOcupacional = (tCliCategoriaOcupacional *)NULL;

	fml = (FBFR32 *)rqst->data;
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	if ((listaCategoriaOcupacional = QNew_(10,10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecuperarCategoriaOcupacional(listaCategoriaOcupacional);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen Categorias ocupacionales", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlExcRecuperarCategoriaOcupacional", 0);
			userlog("%s: Error en funcion SqlExcRecuperarCategoriaOcupacional.", rqst->name);
			QDelete(listaCategoriaOcupacional);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	fml = NewFml32((long)listaCategoriaOcupacional->count, (short)1, (short)0, (short)1, (long)230);
	
	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaCategoriaOcupacional);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY,rqst->name), (char *)fml, 0L, 0L);
	}
	
	for (i=0; i < listaCategoriaOcupacional->count; i++)
	{
		categoriaOcupacional = (tCliCategoriaOcupacional *) QGetItem(listaCategoriaOcupacional, i);
		
		Fadd32(fml, CLI_COD_OCUPA,        (char *)&categoriaOcupacional->codigo         ,           i);
		Fadd32(fml, CLI_DES_OCUPA,        categoriaOcupacional->descripcion             ,           i);
	}
	
	QDelete(listaCategoriaOcupacional);
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliSegCli
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    21-11-2007
  *  DESCRIPTION
  *    Segmentar un Cliente
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliSegCli(TPSVCINFO *rqst)
{
	FBFR32				*fml;
	int				transaccionGlobal=0;
	long				sts=0;
	long				rut=0;
	long				ejecutivo=0;
	char				fechaNacimiento[8 + 1];
	char				modoSegmentacion[10 + 1];
	short				nivelEducacional=0;
	short				categoriaOcupacional=0;
	short				edad=0;
	short				codigoRango=0;
	short				codigoGSE=0;
	short				codigoSegmentoComercial=0;
	tCliMatrizSegmentoComercial	matriz;
	
	fechaNacimiento[CLI_SEG_CERO] = '\0';
	modoSegmentacion[CLI_SEG_CERO] = '\0';

	memset((void *)&matriz,0,sizeof(tCliMatrizSegmentoComercial));
	
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_FECHANACIMIENTO,0,      fechaNacimiento                         ,    0);
	Fget32(fml, CLI_EDAD,0,                 (char *)&edad                           ,    0);
	Fget32(fml, CLI_COD_NIVEL,0,            (char *)&nivelEducacional               ,    0);
	Fget32(fml, CLI_COD_OCUPA,0,            (char *)&categoriaOcupacional           ,    0);
	Fget32(fml, CLI_SEG_EJE,0,              (char *)&ejecutivo                      ,    0);
	Fget32(fml, CLI_MODO_SEG,0,             modoSegmentacion                        ,    0);
	Fget32(fml, CLI_RUT,0,                  (char *)&rut                            ,    0);
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts = SqlCliRecuperarRangoEdad(edad,&codigoRango);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe rango para esta edad ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarRangoEdad ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarRangoEdad.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	/* Recuperacion de SegmentoComercial y GSE */
	sts = SqlCliRecuperarSegmentoGSE(codigoRango,nivelEducacional,categoriaOcupacional
	,&codigoSegmentoComercial,&codigoGSE);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe codigo para este filtro ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarSegmentoGSE ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarCodigoMatriz.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	sts = SqlCliActualizarDatosCliente(rut,fechaNacimiento,nivelEducacional,codigoGSE);
	
	if (sts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al actualizar datos del cliente ", 0);
		userlog("%s: Error en funcion SqlCliActualizarDatosCliente.", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	}
	
	matriz.codigoSegmentoComercial = codigoSegmentoComercial ;
	matriz.codigoNivelEducacional = nivelEducacional;
	matriz.codigoCategoriaOcupacional = categoriaOcupacional;
	matriz.codigoRangoEdad = codigoRango ;
	
	sts = SqlCliRecuperarDatosMatriz(&matriz);
	
	if (sts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarDatosMatriz ", 0);
		userlog("%s: Error en funcion SqlCliRecuperarDatosMatriz.", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecuperarRegistroHistoricoSegmentacion(rut);
	
	if (sts != SQL_SUCCESS)
	{
		if (sts == SQL_NOT_FOUND)
		{
			sts = SqlCliInsertarHistoricoSegmentacion(rut,ejecutivo,modoSegmentacion,&matriz);
		
			if (sts != SQL_SUCCESS)
			{
				Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliInsertarHistoricoSegmentacion ", 0);
				userlog("%s: Error en funcion SqlCliInsertarHistoricoSegmentacion.", rqst->name);
				TRX_ABORT(transaccionGlobal);
				tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
			}
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarRegistroHistoricoSegmentacion ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarRegistroHistoricoSegmentacion.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	else
	{
	
		sts = SqlCliActualizarHistoricoSegmentacion(rut,ejecutivo,modoSegmentacion,&matriz);
		
		if (sts != SQL_SUCCESS)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliActualizarHistoricoSegmentacion ", 0);
			userlog("%s: Error en funcion SqlCliActualizarHistoricoSegmentacion.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
		
	}
	
	Fadd32(fml, CLI_COD_SEG_COM,(char *)&codigoSegmentoComercial,        0);
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/****s* cliente/servidores/fuentes
  *  NAME
  *    CliCalculaMat
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    28-11-2007
  *  DESCRIPTION
  *    Calcular el segmento comercial dado un filtro (rango edad,nivel educacional y categoria ocupacional) 
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliCalculaMat(TPSVCINFO *rqst)
{
	FBFR32                  *fml;
	int                     transaccionGlobal=0;
	long                    sts=0;
	short                   nivelEducacional=0;
	short			categoriaOcupacional=0;
	short			edad=0;
	short			codigoGse=0;
	short                   codigoRango=0;
	short			codigoSegmentoComercial=0;
	
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_EDAD,0,          (char *)&edad                ,    0);
	Fget32(fml, CLI_COD_NIVEL,0,     (char *)&nivelEducacional    ,    0);
	Fget32(fml, CLI_COD_OCUPA,0,     (char *)&categoriaOcupacional,    0);
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts = SqlCliRecuperarRangoEdad(edad,&codigoRango);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe rango para esta edad ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarRangoEdad ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarRangoEdad.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	sts = SqlCliRecuperarSegmentoGSE(codigoRango,nivelEducacional,categoriaOcupacional
	,&codigoSegmentoComercial,&codigoGse);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe codigo para este filtro ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarSegmentoGSE ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarCodigoMatriz.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	Fadd32(fml, CLI_COD_SEG_COM,(char *)&codigoSegmentoComercial,        0);
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecHisSeg
  *  AUTHOR
  *    Cristian Osorio Iribarren
  *  CREATION DATE
  *    29-11-2007
  *  DESCRIPTION
  *    Recuperar el historico de segmentaciones de un cliente
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecHisSeg(TPSVCINFO *rqst)
{
	FBFR32				*fml;
	int				transaccionGlobal=0;
	long				sts=0;
	long				rut=0;
	Q_HANDLE			*listaHistoricoSegmentacion;
	int				i=0;
	tCliHistoricoSegmentacion	*historicoSegmentacion;
	
	historicoSegmentacion = (tCliHistoricoSegmentacion *)NULL;

	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_RUT,0,    (char *)&rut        ,    0);
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	if ((listaHistoricoSegmentacion = QNew_(10,10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
	}
	
	sts = SqlCliRecuperarHistoricoSegmentacion(rut,listaHistoricoSegmentacion);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen Historico para este rut", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarHistoricoSegmentacion", 0);
			userlog("%s: Error en funcion SqlCliRecuperarHistoricoSegmentacion.", rqst->name);
			QDelete(listaHistoricoSegmentacion);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	
	fml = NewFml32((long)listaHistoricoSegmentacion->count, (short)10, (short)0, (short)6, (long)150);
	
	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaHistoricoSegmentacion);
		TRX_ABORT(transaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY,rqst->name), (char *)fml, 0L, 0L);
	}
	
	for (i=0; i < listaHistoricoSegmentacion->count; i++)
	{
		historicoSegmentacion = (tCliHistoricoSegmentacion *) QGetItem(listaHistoricoSegmentacion, i);
		
		Fadd32(fml, CLI_RUT,            (char *)&rut                                                 ,       i);
		Fadd32(fml, CLI_COD_HIS_SEG,    (char *)&historicoSegmentacion->codigoHistoricoSegmentacion  ,       i);
		Fadd32(fml, CLI_FECHA_HIS,      historicoSegmentacion->fechaSegmentacion                     ,       i);
		Fadd32(fml, CLI_COD_MAT_SEG,    (char *)&historicoSegmentacion->codigoMatrizSegmentoComercial,       i);
		Fadd32(fml, CLI_COD_SEG_COM,    (char *)&historicoSegmentacion->codigoSegmentoComercial      ,       i);
		Fadd32(fml, CLI_SEG_DESCRIPCION,historicoSegmentacion->nombre                                ,       i);
		Fadd32(fml, CLI_RANGO_EDAD,     (char *)&historicoSegmentacion->codigoRangoEdad              ,       i);
		Fadd32(fml, CLI_RANGO_MIN,      (char *)&historicoSegmentacion->rangoMinimo                  ,       i);
		Fadd32(fml, CLI_RANGO_MAX,      (char *)&historicoSegmentacion->rangoMaximo                  ,       i);
		Fadd32(fml, CLI_COD_NIVEL,      (char *)&historicoSegmentacion->codigoNivelEducacional       ,       i);
		Fadd32(fml, CLI_DES_NIVEL,      historicoSegmentacion->descripcionNivelEducacional           ,       i);
		Fadd32(fml, CLI_COD_OCUPA,      (char *)&historicoSegmentacion->codigoCategoriaOcupacional   ,       i);
		Fadd32(fml, CLI_DES_OCUPA,      historicoSegmentacion->descripcionCortaCatOcu                ,       i);
		Fadd32(fml, CLI_SEG_EJE,        (char *)&historicoSegmentacion->codigoEjecutivo              ,       i);
		Fadd32(fml, CLI_TIPO_SEG,       historicoSegmentacion->tipoSegmentacion                      ,       i);
		Fadd32(fml, CLI_ACTIVO_SEG,     historicoSegmentacion->activo                                ,       i);
	}
	
	QDelete(listaHistoricoSegmentacion);
	TRX_COMMIT(transaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecOcuCli
  *  AUTHOR
  *    Susana Gonzalez 
  *  CREATION DATE
  *    13-12-2007
  *  DESCRIPTION
  *    Recuperar la categoria ocupacional de un cliente
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecOcuCli(TPSVCINFO *rqst)
{
	FBFR32		*fml;
	int		transaccionGlobal=0;
	long		rut=0;
	long		sts=0;
	short		codigoOcu=0;
	char		descripcion[CLI_SEG_DESCRIPCIONLARGA];
	
	descripcion[CLI_SEG_CERO] = '\0';
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_RUT,0,          (char *)&rut                ,    0);
	
	transaccionGlobal = tpgetlev();
	TRX_BEGIN(transaccionGlobal);
	
	sts = SqlCliRecuperarCatOcuCliente(rut, descripcion,&codigoOcu);
	
	if (sts != SQL_SUCCESS)
	{
		if ( sts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe datos de Categoria Ocupacional para el cliente ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion SqlCliRecuperarCatOcuCliente ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarCatOcuCliente.", rqst->name);
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		}
	}
	else
	{
		Fadd32(fml, CLI_COD_OCUPA,(char *)&codigoOcu,        0);
		Fadd32(fml, CLI_DES_OCUPA,descripcion,        0);
		TRX_COMMIT(transaccionGlobal);
		tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
	}
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliModCatOcu 
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    02-01-2008
  *  DESCRIPTION
  *    Modificar datos de la categoria ocupacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliModCatOcu(TPSVCINFO *rqst)
{
	FBFR32	*fml;
	int	intTransaccionGlobal=0;
	long	lngSts=0;

	tCliCategoriaOcupacional	categoriaOcupacional;

	/******   Buffer de entrada   ******/
	memset((void *)&categoriaOcupacional,0,sizeof(tCliCategoriaOcupacional));
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_OCUPA,0,  (char *)&categoriaOcupacional.codigo, 0);
	Fget32(fml, CLI_DES_OCUPA,0,  categoriaOcupacional.descripcion, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlModificarCategoriaOcup(&categoriaOcupacional); 

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar Categoria Ocupacional", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecCatOcuX
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    02-01-2008
  *  DESCRIPTION
  *    Recuperar la categoria ocupacional dado su codigo
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecCatOcuX(TPSVCINFO *rqst)
{
	FBFR32       *fml;
	int          intTransaccionGlobal=0;
	long         lngSts=0;
	tCliCategoriaOcupacional   categoriaOcupacional;

	memset((void *)&categoriaOcupacional,0,sizeof(tCliCategoriaOcupacional));
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_OCUPA,0,  (char *)&categoriaOcupacional.codigo, 0);

	/******   Cuerpo del servicio   ******/
	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliRecuperarCategoriaOcup(&categoriaOcupacional);
	if (lngSts != SQL_SUCCESS)
	{
		if ( lngSts == SQL_NOT_FOUND )
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una categoria para este codigo ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcionSqlCliRecuperarCategoriaOcup ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarCategoriaOcup.", rqst->name);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name),(char *)fml, 0L, 0L);
		}
	}

	Fadd32(fml, CLI_DES_OCUPA,categoriaOcupacional.descripcion,           0);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliInsCatOcu
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    02-01-2008
  *  DESCRIPTION
  *    Insertar una categoria ocupacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliInsCatOcu(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliCategoriaOcupacional categoriaOcupacional;

	memset((void *)&categoriaOcupacional,0,sizeof(tCliCategoriaOcupacional));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_OCUPA,0,  (char *)&categoriaOcupacional.codigo, 0);
	Fget32(fml, CLI_DES_OCUPA,0,  categoriaOcupacional.descripcion, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliInsertarCategoriaOcup(&categoriaOcupacional); 

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_DUPLICATE_KEY)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "categoriaOcupacional ya se encuentra definido" , 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear categoriaOcupacional", 0);
		}

		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliEliCatOcu
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    02-01-2008
  *  DESCRIPTION
  *    Eliminar una categoria ocupacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliEliCatOcu(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliCategoriaOcupacional categoriaOcupacional;

	memset((void *)&categoriaOcupacional,0,sizeof(tCliCategoriaOcupacional));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_OCUPA,0,  (char *)&categoriaOcupacional.codigo, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliEliminarCategoriaOcup (&categoriaOcupacional); 

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar Categoria Ocupacional", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecRanEda
  *  AUTHOR
  *    Alexis Munoz Acuna
  *  CREATION DATE
  *    15-01-2008
  *  DESCRIPTION
  *    Recuperar Todos los datos de la tabla Rango Edad
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecRanEda(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	Q_HANDLE *listaRangoEdad;
	int i=0;
	tCliRangoEdad *rangoEdad;

	rangoEdad = (tCliRangoEdad *)NULL;

	fml = (FBFR32 *)rqst->data;

	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	if ((listaRangoEdad = QNew_(10, 10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,
				0L, 0L);
	}

	lngSts = SqlCliRecuperarRangoEdadX(listaRangoEdad);

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen Rango Edad", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error en funcion SqlCliRecuperarRangoEdadX", 0);
			userlog("%s: Error en funcion SqlCliRecuperarRangoEdadX.",
					rqst->name);
			QDelete(listaRangoEdad);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,
					0L);
		}
	}

	fml = NewFml32((long)listaRangoEdad->count, (short)3, (short)0, (short)2,
			(long)100);

	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaRangoEdad);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,
				0L, 0L);
	}

	for (i=0; i < listaRangoEdad->count; i++)
	{
		rangoEdad = (tCliRangoEdad *) QGetItem(listaRangoEdad, i);

		Fadd32(fml, CLI_COD_RANGO, (char *)&rangoEdad->codigo , i);
		Fadd32(fml, CLI_RANGO_MIN, (char *)&rangoEdad->edadMinima , i);
		Fadd32(fml, CLI_RANGO_MAX, (char *)&rangoEdad->edadMaxima , i);
		Fadd32(fml, CLI_DES_RANGO, rangoEdad->descripcion, i);
		Fadd32(fml, CLI_CAR_RANGO, rangoEdad->caracteristica, i);
	}

	QDelete(listaRangoEdad);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliRecGse
  *  AUTHOR
  *    Alexis Munoz Acuna
  *  CREATION DATE
  *    15-01-2008
  *  DESCRIPTION
  *    Recuperar Todos los datos de la tabla GSE
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecGse(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	Q_HANDLE *listaGrupoSocioEconomico;
	int i=0;
	tCliGrupoSocioEconomico *grupoSocioEconomico;

	grupoSocioEconomico = (tCliGrupoSocioEconomico *)NULL;

	fml = (FBFR32 *)rqst->data;

	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	if ((listaGrupoSocioEconomico = QNew_(10, 10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,
				0L, 0L);
	}

	lngSts = SqlCliRecuperarGrupoSocioEconomicoX(listaGrupoSocioEconomico);

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"No existen Grupo Socio Economico", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error en funcion SqlCliRecuperarGrupoSocioEconomicoX", 0);
			userlog(
					"%s: Error en funcion SqlCliRecuperarGrupoSocioEconomicoX.",
					rqst->name);
			QDelete(listaGrupoSocioEconomico);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,
					0L);
		}
	}

	fml = NewFml32((long)listaGrupoSocioEconomico->count, (short)1, (short)0,
			(short)2, (long)110);

	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaGrupoSocioEconomico);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,
				0L, 0L);
	}

	for (i=0; i < listaGrupoSocioEconomico->count; i++)
	{
		grupoSocioEconomico = (tCliGrupoSocioEconomico *) QGetItem(
				listaGrupoSocioEconomico, i);

		Fadd32(fml, CLI_COD_GSE, (char *)&grupoSocioEconomico->codigo , i);
		Fadd32(fml, CLI_DES_GSE, grupoSocioEconomico->descripcion, i);
		Fadd32(fml, CLI_CAR_GSE, grupoSocioEconomico->caracteristicas, i);
	}

	QDelete(listaGrupoSocioEconomico);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecRanEdaX
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Recuperar el rango edad dado su codigo
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecRanEdaX(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	tCliRangoEdad rangoEdad;

	memset((void *)&rangoEdad, 0, sizeof(tCliRangoEdad));
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_RANGO, 0, (char *)&rangoEdad.codigo, 0);

	/******   Cuerpo del servicio   ******/
	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliRecuperarRangoedad(&rangoEdad);
	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"No existe un rango edad para este codigo ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error en funcion SqlCliRecuperarRangoedad ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarRangoedad.",
					rqst->name);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,
					0L);
		}
	}

	Fadd32(fml, CLI_RANGO_MIN, (char *)&rangoEdad.edadMinima, 0);
	Fadd32(fml, CLI_RANGO_MAX, (char *)&rangoEdad.edadMaxima, 0);
	Fadd32(fml, CLI_DES_RANGO, rangoEdad.descripcion, 0);
	Fadd32(fml, CLI_CAR_RANGO, rangoEdad.caracteristica, 0);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecGseX
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Recuperar el GSE dado su codigo
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecGseX(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	tCliGrupoSocioEconomico grupoSocioEconomico;

	memset((void *)&grupoSocioEconomico, 0, sizeof(tCliGrupoSocioEconomico));
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_GSE, 0, (char *)&grupoSocioEconomico.codigo, 0);

	/******   Cuerpo del servicio   ******/
	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliRecuperarGrupoSocioEconomico(&grupoSocioEconomico);
	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"No existe un grupo Socio Economico para este codigo ", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error en funcion SqlCliRecuperarGrupoSocioEconomico ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarGrupoSocioEconomico.",
					rqst->name);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,
					0L);
		}
	}

	Fadd32(fml, CLI_DES_GSE, grupoSocioEconomico.descripcion, 0);
	Fadd32(fml, CLI_CAR_GSE, grupoSocioEconomico.caracteristicas, 0);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliInsRanEda
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Insertar un rango edad
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliInsRanEda(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliRangoEdad rangoEdad;

	memset((void *)&rangoEdad, 0, sizeof(tCliRangoEdad));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_RANGO, 0, (char *)&rangoEdad.codigo, 0);
	Fget32(fml, CLI_RANGO_MIN, 0, (char *)&rangoEdad.edadMinima, 0);
	Fget32(fml, CLI_RANGO_MAX, 0, (char *)&rangoEdad.edadMaxima, 0);
	Fget32(fml, CLI_DES_RANGO, 0, rangoEdad.descripcion, 0);
	Fget32(fml, CLI_CAR_RANGO, 0, rangoEdad.caracteristica, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliInsertarRangoEdad(&rangoEdad);

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_DUPLICATE_KEY)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"rango Edad ya se encuentra definido", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear rango Edad", 0);
		}

		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliInsGse
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Insertar un Grupo Socio Economico
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliInsGse(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliGrupoSocioEconomico grupoSocioEconomico;

	memset((void *)&grupoSocioEconomico, 0, sizeof(tCliGrupoSocioEconomico));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_GSE, 0, (char *)&grupoSocioEconomico.codigo, 0);
	Fget32(fml, CLI_DES_GSE, 0, grupoSocioEconomico.descripcion, 0);
	Fget32(fml, CLI_CAR_GSE, 0, grupoSocioEconomico.caracteristicas, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliInsertarGrupoSocioEconomico(&grupoSocioEconomico);

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_DUPLICATE_KEY)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					" Grupo Socio Economico ya se encuentra definido", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error al crear Grupo Socio Economico", 0);
		}

		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliEliRanEda
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Eliminar una rango Edad 
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliEliRanEda(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliRangoEdad rangoEdad;

	memset((void *)&rangoEdad, 0, sizeof(tCliRangoEdad));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_RANGO, 0, (char *)&rangoEdad.codigo, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliEliminarRangoEdad(&rangoEdad);

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar Rango Edad", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliEliGse
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Eliminar un Grupo Socio Economico
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliEliGse(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliGrupoSocioEconomico grupoSocioEconomico;

	memset((void *)&grupoSocioEconomico, 0, sizeof(tCliGrupoSocioEconomico));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_GSE, 0, (char *)&grupoSocioEconomico.codigo, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliEliminarGrupoSocioEconomico(&grupoSocioEconomico);

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error al eliminar Grupo Socio Economico", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliModRanEda
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Modificar datos del rango Edad 
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliModRanEda(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliRangoEdad rangoEdad;

	/******   Buffer de entrada   ******/
	memset((void *)&rangoEdad, 0, sizeof(tCliRangoEdad));
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_RANGO, 0, (char *)&rangoEdad.codigo, 0);
	Fget32(fml, CLI_RANGO_MIN, 0, (char *)&rangoEdad.edadMinima, 0);
	Fget32(fml, CLI_RANGO_MAX, 0, (char *)&rangoEdad.edadMaxima, 0);
	Fget32(fml, CLI_DES_RANGO, 0, rangoEdad.descripcion, 0);
	Fget32(fml, CLI_CAR_RANGO, 0, rangoEdad.caracteristica, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlModificarRangoEdad(&rangoEdad);

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar Rango Edad", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliModGse
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    16-01-2008
  *  DESCRIPTION
  *    Modificar datos de la Grupo Socio Economico
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliModGse(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliGrupoSocioEconomico grupoSocioEconomico;

	/******   Buffer de entrada   ******/
	memset((void *)&grupoSocioEconomico, 0, sizeof(tCliGrupoSocioEconomico));
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_COD_GSE, 0, (char *)&grupoSocioEconomico.codigo, 0);
	Fget32(fml, CLI_DES_GSE, 0, grupoSocioEconomico.descripcion, 0);
	Fget32(fml, CLI_CAR_GSE, 0, grupoSocioEconomico.caracteristicas, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlModificarGrupoSocioEconomico(&grupoSocioEconomico);

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO,
				"Error al modificar Grupo Socio Economico", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecNivEduX
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    17-01-2008
  *  DESCRIPTION
  *    Recuperar el nivel educacional dado su codigo
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecNivEduX(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	tCliCODIGODESCRIPCION nivelEducacional;

	memset((void *)&nivelEducacional, 0, sizeof(tCliCODIGODESCRIPCION));
	/***   Buffer de entrada   ***/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_TABLA_CODIGO, 0, (char *)&nivelEducacional.codigo, 0);

	/******   Cuerpo del servicio   ******/
	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliRecuperarNivelEducacional(&nivelEducacional);
	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"No existe un Nivel Educacional para este codigo ", 0);
		} else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,
					"Error en funcion SqlCliRecuperarNivelEducacional ", 0);
			userlog("%s: Error en funcion SqlCliRecuperarNivelEducacional.",
					rqst->name);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,
					0L);
		}
	}

	Fadd32(fml, CLI_TABLA_DESCRIPCION, nivelEducacional.descripcion, 0);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliInsNivEduc
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    17-01-2008
  *  DESCRIPTION
  *    Insertar un nivel educacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliInsNivEduc(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliCODIGODESCRIPCION nivelEducacional;

	memset((void *)&nivelEducacional,0,sizeof(tCliCODIGODESCRIPCION));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_TABLA_CODIGO,0,  (char *)&nivelEducacional.codigo, 0);
	Fget32(fml, CLI_TABLA_DESCRIPCION,0,  nivelEducacional.descripcion, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();
	
	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliInsertarNivelEducacional(&nivelEducacional); 
	
	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_DUPLICATE_KEY)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "nivel educacional  ya se encuentra definido" , 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear nivel educacional", 0);
		}
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *    CliEliNivEduc
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    17-01-2008
  *  DESCRIPTION
  *    Eliminar un nivel educacional
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliEliNivEduc(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	
	tCliCODIGODESCRIPCION nivelEducacional;
	
	memset((void *)&nivelEducacional,0,sizeof(tCliCODIGODESCRIPCION));
	
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_TABLA_CODIGO,0,  (char *)&nivelEducacional.codigo, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliEliminarNivelEducacional(&nivelEducacional); 

	if (lngSts != SQL_SUCCESS)
	{
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar Nivel Educacional", 0);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
	
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliModNivEduc
  *  AUTHOR
  *    Alexis Muñoz
  *  CREATION DATE
  *    17-01-2008
  *  DESCRIPTION
  *    Modificar datos del nivel educacional
  *  PARAMETERS
  *
  */

void CliModNivEduc(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliCODIGODESCRIPCION nivelEducacional;

	/****** Buffer de entrada ******/
	memset((void *)&nivelEducacional,0,sizeof(tCliCODIGODESCRIPCION));
	fml = (FBFR32 *)rqst->data;

	Fget32(fml, CLI_TABLA_CODIGO,0, (char *)&nivelEducacional.codigo, 0);
	Fget32(fml, CLI_TABLA_DESCRIPCION,0, nivelEducacional.descripcion, 0);

	/****** Cuerpo del servicio ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliModificarNivelEducacional(&nivelEducacional);

	if (lngSts != SQL_SUCCESS)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar nivel educacional", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecSegTodos
  *  AUTHOR
  *    Cristian Osorio 
  *  CREATION DATE
  *    18-01-2008
  *  DESCRIPTION
  *    Recuperar todos los segmentos comerciales
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecSegTodos(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	Q_HANDLE *listaSegmentosComerciales;
	int i=0;
	tCliSegmentacionComercial *segmentoComercial;

	segmentoComercial = (tCliSegmentacionComercial *)NULL;

	fml = (FBFR32 *)rqst->data;

	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);

	if ((listaSegmentosComerciales = QNew_(10, 10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,0L, 0L);
	}

	lngSts = SqlCliRecuperarTodosSegmentacionComercial(listaSegmentosComerciales);

	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,"No existen Segmentos Comerciales", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion SqlCliRecuperarTodosSegmentacionComercial", 0);
			userlog("%s: Error en funcion SqlCliRecuperarTodosSegmentacionComercial.",rqst->name);
			QDelete(listaSegmentosComerciales);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,0L);
		}
	}

	fml = NewFml32((long)listaSegmentosComerciales->count, (short)1, (short)0,(short)1, (long)120);

	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaSegmentosComerciales);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,0L, 0L);
	}

	for (i=0; i < listaSegmentosComerciales->count; i++)
	{
		segmentoComercial = (tCliSegmentacionComercial *) QGetItem(listaSegmentosComerciales, i);
		Fadd32(fml, CLI_COD_SEG_COM,(char *)&segmentoComercial->codigo ,i);
		Fadd32(fml, CLI_DES_GSE, segmentoComercial->rangoGse , i);
	}

	QDelete(listaSegmentosComerciales);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****s* cliente/servidores/fuentes
  *  NAME
  *  CliRecComSeg
  *  AUTHOR
  *    Cristian Osorio
  *  CREATION DATE
  *    21-01-2008
  *  DESCRIPTION
  *    Recuperar Combinaciones (rango de edad, categoria ocupacional y nivel educacional) de un segmento Comercial
  *  PARAMETERS
  *
  *  RETURN VALUE
  *
  *  MODIFICATION HISTORY
  *
  ******
  *
  *
  */

void CliRecComSeg(TPSVCINFO *rqst)
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
	Q_HANDLE *listaSegmentosComerciales;
	int i=0;
	tCliCombinacionSegmentoComercial *segmentoComercial;
	short intCodigoSegmento=0;
	
	segmentoComercial = (tCliCombinacionSegmentoComercial *)NULL;

	fml = (FBFR32 *)rqst->data;
	
	Fget32(fml, CLI_COD_SEG_COM,0, (char *)&intCodigoSegmento, 0);
	
	intTransaccionGlobal = tpgetlev();
	TRX_BEGIN(intTransaccionGlobal);
	
	if ((listaSegmentosComerciales = QNew_(10, 10)) == NULL)
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en QNew_", 0);
		userlog("%s: Error en QNew_", rqst->name);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,0L, 0L);
	}
	
	lngSts = SqlCliRecuperarCombSegmentoComercial(intCodigoSegmento,listaSegmentosComerciales);
	
	if (lngSts != SQL_SUCCESS)
	{
		if (lngSts == SQL_NOT_FOUND)
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,"No datos para este segmento comercial", 0);
		}
		else
		{
			Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion SqlCliRecuperarCombSegmentoComercial", 0);
			userlog("%s: Error en funcion SqlCliRecuperarCombSegmentoComercial.",rqst->name);
			QDelete(listaSegmentosComerciales);
			TRX_ABORT(intTransaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L,0L);
		}
	}

	fml = NewFml32((long)listaSegmentosComerciales->count, (short)0, (short)0,(short)4, (long)120);

	if (fml == NULL)
	{
		fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en funcion NewFml32", 0);
		userlog("%s: Error en funcion NewFml32.", rqst->name);
		QDelete(listaSegmentosComerciales);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml,0L, 0L);
	}

	for (i=0; i < listaSegmentosComerciales->count; i++)
	{
		segmentoComercial = (tCliCombinacionSegmentoComercial *) QGetItem(listaSegmentosComerciales, i);
		Fadd32(fml, CLI_DES_GSE, (char *)&segmentoComercial->rangoGse , i);
		Fadd32(fml, CLI_DES_RANGO, (char *)&segmentoComercial->rangoEdad , i);
		Fadd32(fml, CLI_DES_OCUPA, (char *)&segmentoComercial->categoriaOcupacional , i);
		Fadd32(fml, CLI_DES_NIVEL, (char *)&segmentoComercial->nivelEducacional , i);
	}

	QDelete(listaSegmentosComerciales);
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/********************************************************************************/
/* Objetivo: Recupera los Mensajes Popup vigentes a mostrar al cliente          */
/* Autor   : Juan Carlos Meza                     Fecha: 22 octubre 2009        */
/*         :                                                                    */
/* Modifica:                                      Fecha:                        */
/********************************************************************************/
void CliRecMsjPopVig(TPSVCINFO *rqst)
{
   FBFR32   *fml=NULL;
   int      transaccionGlobal=0;
   long     sts=0;
   tCliMENSAJEPOPUP *mensajePopup;
   Q_HANDLE *lista;
   long     rutCliente, codigoUsuario;
   int      totalRegistros=0, i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&codigoUsuario,0);
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliValidarSiPerteneceAGrupoMensajePopup(codigoUsuario);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si usuario pertenece a Grupo de Usuarios que muestran mensajes popup", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   else if (sts == SQL_NOT_FOUND)
   {
      sts = SqlCliValidarSiPerteneceAUsuarioMensajePopup(codigoUsuario);
      if (sts == SQL_NOT_FOUND)
      {
         /* Si codigoUsuario no pertenece a GrupoMensajePopup ni a UsuarioMensajePopup,
            entonces salimos con exito sin mostrar mensajes */
         TRX_COMMIT(transaccionGlobal);
         tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
      }
      else if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si Usuario pertenece a Usuarios que muestran mensajes popup", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

   /* Se retornara mensajes si los hay */

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al asignar memoria a lista de mensajes popup.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

/********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
/********* ContinuidadHoraria [FIN] *********/

   sts = SqlCliRecuperarMensajesPopupVigentes(rutCliente, gFechaProceso, lista);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar mensajes popup vigentes", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros, sizeof(tCliMENSAJEPOPUP)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      mensajePopup = (tCliMENSAJEPOPUP *) QGetItem(lista, i);

      Fadd32(fml, CLI_DESCRIPCION, mensajePopup->mensaje, i);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Inserta grupo hermes que tiene acceso al mensaje popup           */
/* Autor   : Juan Carlos Meza                     Fecha: 23 octubre 2009      */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliInsGrpMsjPop(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliGRUPOMENSAJEPOPUP grupoMensajePopup;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO_GRUPO, 0, (char *)&grupoMensajePopup.grupo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliValidarExistenciaGrupoHermes(&grupoMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Grupo especificado no existe", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliValidarExistenciaGrupoHermes", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliInsertarGrupoMensajePopup(&grupoMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Grupo Mensaje Popup ya existe", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliInsertarGrupoMensajePopup", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***************************************************************************/
/* Objetivo: Elimina grupo hermes que tiene acceso al mensaje popup        */
/* Autor   : Juan Carlos Meza                 Fecha: 23 octubre 2009       */
/* Modifica:                                  Fecha:                       */
/***************************************************************************/
void CliElmGrpMsjPop(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliGRUPOMENSAJEPOPUP grupoMensajePopup;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_CODIGO_GRUPO, 0, (char *)&grupoMensajePopup.grupo, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliEliminarGrupoMensajePopup(&grupoMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Grupo indicado no ve mensajes popup" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliEliminarGrupoMensajePopup", 0);
  
      TRX_COMMIT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************************************************************************************/
/* Objetivo: Recupera lista de grupos hermes que tienen acceso a ver los Mensajes Popup */
/* Autor   : Juan Carlos Meza                     Fecha: 22 octubre 2009                */
/*         :                                                                            */
/* Modifica:                                      Fecha:                                */
/****************************************************************************************/
void CliLisGrpMsjPop(TPSVCINFO *rqst)
{
   FBFR32   *fml=NULL;
   int      transaccionGlobal=0;
   long     sts=0;
   tCliGRUPOMENSAJEPOPUP *grupoMensajePopup;
   Q_HANDLE *lista;
   int      totalRegistros=0, i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al asignar memoria a lista de mensajes popup.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarGrupoMensajePopupTodos(lista);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarGrupoMensajePopupTodos", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros, sizeof(tCliGRUPOMENSAJEPOPUP)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      grupoMensajePopup = (tCliGRUPOMENSAJEPOPUP *) QGetItem(lista, i);

      Fadd32(fml, CLI_CODIGO_GRUPO, (char *)&grupoMensajePopup->grupo, i);
      Fadd32(fml, CLI_NOMBRE, grupoMensajePopup->nombre, i);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Inserta usuario hermes que tiene acceso al mensaje popup         */
/* Autor   : Juan Carlos Meza                     Fecha: 23 octubre 2009      */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliInsUsuMsjPop(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliUSUARIOMENSAJEPOPUP usuarioMensajePopup;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_USUARIO, 0, (char *)&usuarioMensajePopup.usuario, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliInsertarUsuarioMensajePopup(&usuarioMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario Mensaje Popup ya existe", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliInsertaUsuarioMensajePopup", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***************************************************************************/
/* Objetivo: Elimina usuario hermes que tiene acceso al mensaje popup      */
/* Autor   : Juan Carlos Meza                 Fecha: 23 octubre 2009       */
/* Modifica:                                  Fecha:                       */
/***************************************************************************/
void CliElmUsuMsjPop(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliUSUARIOMENSAJEPOPUP usuarioMensajePopup;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_USUARIO, 0, (char *)&usuarioMensajePopup.usuario, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliEliminarUsuarioMensajePopup(&usuarioMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no ve mensajes popup" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliEliminarUsuarioMensajePopup", 0);
  
      TRX_COMMIT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/********************************************************************************************************/
/* Objetivo: Recupera usuario hermes por su nombre/login indicando si tiene acceso a ver mensajes popup */
/* Autor   : Juan Carlos Meza                 Fecha: 23 octubre 2009                                    */
/* Modifica:                                  Fecha:                                                    */
/********************************************************************************************************/
void CliRecUsuMsjPop(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliUSUARIOMENSAJEPOPUP usuarioMensajePopup;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_NOMBRE, 0, (char *)&usuarioMensajePopup.nombre, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarUsuarioMensajePopupXNombre(&usuarioMensajePopup);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarUsuarioMensajePopupXNombre", 0);
  
      TRX_COMMIT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_USUARIO, (char *)&usuarioMensajePopup.usuario, 0);
   Fadd32(fml, CLI_DESCRIPCION, usuarioMensajePopup.descripcion, 0);
   Fadd32(fml, CLI_PUEDE_VER, usuarioMensajePopup.puedeVer, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Inserta cliente en tabla CLIENTEZONARIESGO                       */
/* Autor   : Susana Gonzalez                      Fecha: 16 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliInsZonaRiesg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliInsertarClienteZonaRiesgo(Rut);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente Existe en Zona de Riesgo ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliInsertarClienteZonaRiesgo", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Recupera cliente en tabla CLIENTEZONARIESGO                      */
/* Autor   : Susana Gonzalez                      Fecha: 17 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliRecZonaRiesg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;
   long existe=1L;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClienteZonaRiesgo(Rut);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
		existe = 0L;
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarClienteZonaRiesgo", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
   }


   Fadd32(fml, CLI_EXISTE_CLIENTE, (char *)&existe , 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Elimina cliente en tabla CLIENTEZONARIESGO                       */
/* Autor   : Susana Gonzalez                      Fecha: 17 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliElmZonaRiesg(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliEliminarClienteZonaRiesgo(Rut);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no eiste en Zona de Riesgo " , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliEliminarClienteZonaRiesgo", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Inserta cliente en tabla RCLIENTESECTORSII                       */
/* Autor   : Susana Gonzalez                      Fecha: 17 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliInsCliSecSII(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;
   long SectorSII;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);
   Fget32(fml, CLI_SECTOR_SII, 0, (char *)&SectorSII, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliInsertarClienteSectorSII(Rut, SectorSII);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente Existe en Sector SII  ", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliInsertarClienteSectorSII", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************/
/* Objetivo: Modifica cliente en tabla RCLIENTESECTORSII                       */
/* Autor   : Susana Gonzalez                      Fecha: 30 abril 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliModCliSecSII(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;
   long SectorSII;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);
   Fget32(fml, CLI_SECTOR_SII, 0, (char *)&SectorSII, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarClienteSectorSII(Rut, SectorSII);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliModificarClienteSectorSII", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/******************************************************************************/
/* Objetivo: Elimina cliente en tabla RCLIENTESECTORSII                       */
/* Autor   : Susana Gonzalez                      Fecha: 17 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliElmCliSecSII(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliEliminarClienteSectorSII(Rut);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliEliminarClienteSectorSII", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/******************************************************************************/
/* Objetivo: Recupera cliente en tabla RCLIENTESECTORSII                      */
/* Autor   : Susana Gonzalez                      Fecha: 17 marzo 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliRecCliSecSII(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long Rut;
   long SectorSII;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&Rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClienteSectorSII(Rut, &SectorSII);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Sector SII" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarClienteSectorSII", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_SECTOR_SII, (char *)&SectorSII , 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************ SERVICIOS NUEVOS DE REFERENCIA ****************/

/******************************************************************************/
/* Objetivo: Recupera referencia del cliente en tabla CLIENTEREFERENCIA       */
/* Autor   : Christian Macaya Gatica              Fecha: 23 julio 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliRecCliRef(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliREFERENCIA referencia;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&referencia.rutCliente, 0);

    /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClienteReferencia(&referencia);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Referencia" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarClienteReferencia", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUTX, (char *)&referencia.rutCliente, 0);
   Fadd32(fml, CLI_RUT, (char *)&referencia.rutPariente, 0);
   Fadd32(fml, CLI_DIGVER, referencia.dv, 0);
   Fadd32(fml, CLI_CODIGO_PARENTESCO, (char *)&referencia.codigoParentesco, 0);
   Fadd32(fml, CLI_NOMBRES, referencia.nombres, 0);
   Fadd32(fml, CLI_APELLIDO_PATERNO, referencia.apellidoPaterno, 0);
   Fadd32(fml, CLI_APELLIDO_MATERNO, referencia.apellidoMaterno, 0);
   Fadd32(fml, CLI_TIPO_DIRECCION, (char *)&referencia.tipoDireccion, 0);
   Fadd32(fml, CLI_CALLE_NUMERO, referencia.calleNumero, 0);
   Fadd32(fml, CLI_DEPARTAMENTO, referencia.departamento, 0);
   Fadd32(fml, CLI_VILLA_POBLACION, referencia.villaPoblacion, 0);
   Fadd32(fml, CLI_CIUDAD, (char *)&referencia.ciudad, 0);
   Fadd32(fml, CLI_COMUNA, (char *)&referencia.comuna, 0);
   Fadd32(fml, CLI_TIPO_TELEFONO, (char *)&referencia.tipoTelefono, 0);
   Fadd32(fml, CLI_NUMERO, referencia.numero, 0);
   /* ACD */
   Fadd32(fml, CLI_CODIGO_DE_AREA, (char *)&referencia.codigoArea, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Referencia                                           */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Julio   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsReferen(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliREFERENCIA referencia;
   int transaccionGlobal;

   memset((void *)&referencia,0,sizeof(tCliREFERENCIA));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroClienteReferencia(fml, &referencia);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliCrearReferencia(referencia);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La Referencia ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Referencia", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Referencia                                       */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Julio  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModReferen(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliREFERENCIA referencia;
   tCliREFERENCIA referenciaAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroClienteReferencia(fml, &referencia);
   referenciaAnterior = referencia;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarClienteReferencia(&referenciaAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Parentesco.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarReferencia(referencia);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe la Referencia", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
 /*     ConcatenaPariente(referenciaAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_REFERENCIA;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      userlog("correlativo : [%li]", regModificacion.correlativo);
      userlog("usuario : [%li]", regModificacion.usuario);
      userlog("sucursal : [%d]", regModificacion.sucursal);
      userlog("tipoModificacion : [%d]", regModificacion.tipoModificacion);
      userlog("rutCliente : [%li]", regModificacion.rutCliente);
      userlog("fecha : [%s]", regModificacion.fecha);
      userlog("modificacion : [%s]", regModificacion.modificacion);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
*/
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Registros de la Referencia                        */
/* Autor   : Christian Macaya             Fecha: 26 Julio 2010          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmPariente(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long RutCliente;
   int totalRegistros=0;
   char cadena[201];
   tCliREFERENCIA referenciaAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&RutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   referenciaAnterior.rutCliente = RutCliente;

    sts = SqlCliRecuperarClienteReferencia(&referenciaAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Parentesco.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarReferencia(RutCliente);

      if (sts != SQL_SUCCESS)
      {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar la Referencia", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
   {
      /*grabar Reg. Modif.*/
   /*   ConcatenaPariente(referenciaAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_REFERENCIA;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
*/
   }

      TRX_COMMIT(transaccionGlobal);
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************ FIN SERVICIOS NUEVOS DE REFERENCIA ****************/

/*************************************************************************/
/* Objetivo: Recuperar lista de Sectores de SII                          */
/* Autor   : Susana  Gonzalez                         Fecha: 21-04-2010  */
/* Modifica:                                          Fecha:             */
/*************************************************************************/
void CliLisSectorSII(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliSECTORSII         *sectorsii;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarSectorSIITodos(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No hay sectores de SII", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar sectores de SII", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliSECTORSII)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      sectorsii = (tCliSECTORSII  *) QGetItem( lista, i);

      Fadd32(fml, CLI_SECTOR_SII            ,  (char *)&sectorsii->codigo, i);
      Fadd32(fml, CLI_SECTOR_SII_DESCRIPCION,           sectorsii->descripcion , i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/*****************************************/
/* INCORPORACION DEL ESTADO DE SITUACION */
/*****************************************/


/***********************************/
/* GASTOS PASIVOS / BIENES ACTIVOS */
/***********************************/

/************************************************************************************************************/
/* Objetivo: Recupera Bienes Activos y Gastos Pasivos del cliente en tabla GASTOSPASIVOSBIENESACTIVOS       */
/* Autor   : Christian Macaya Gatica              Fecha: 23 marzo 2010                                      */
/* Modifica:                                      Fecha:                                                    */
/************************************************************************************************************/
void CliRecActPasCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   Q_HANDLE *lista;
   short                   vigenciaEESS;
   tCliGASTOSPASIVOSBIENESACTIVOS *gastosPasivosBienesActivos;
   int i;
   char fecha[8+1];
   char fechaEESS[8+1];
   char fechaEESSCliente[8+1];
   char debeActualizar[1+1];
   short diferenciaDias;
   long contador;

   memset(fecha, 0, sizeof(fecha));
   memset(fechaEESS, 0, sizeof(fechaEESS));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(debeActualizar, 0, sizeof(debeActualizar));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);
 
   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "error en QNew_", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlTieneFechaActualizacionEESS(rut,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }

   if (contador > 0){
	sts = SqlRecuperarFechaActualizacionEESS(rut,fechaEESSCliente);
	if (sts != SQL_SUCCESS)
  	{
       		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlRecuperarFechaActualizacionEESS", 0);
       		TRX_ABORT(transaccionGlobal);
       		tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
  	}
   }
   else
   {
	Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe Fecha de Actualizacion EESS", 0);
	TRX_ABORT(transaccionGlobal);
	tpreturn(TPFAIL, ErrorServicio(SQL_NOT_FOUND, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlRecActivoPasivoCli(lista, rut);

   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registros en Activos y/o Pasivos", 0);
      else
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros en Activos y/o Pasivos", 0);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   for (i=0; i<(lista->count); i++)
   {
     gastosPasivosBienesActivos = (tCliGASTOSPASIVOSBIENESACTIVOS *)QGetItem(lista,i);

     Fadd32(fml,CLI_RUTX,(char *)&gastosPasivosBienesActivos->rutCliente, 0);
     Fadd32(fml,CLI_CODIGO_ESTADOSITUACION,(char *)&gastosPasivosBienesActivos->codigo, 0);
     Fadd32(fml,CLI_TIPO_ESTADOSITUACION,(char *)&gastosPasivosBienesActivos->tipo, 0);
     Fadd32(fml,CLI_PAGO_MENSUAL,(char *)&gastosPasivosBienesActivos->pagoMensual, 0);
     Fadd32(fml,CLI_PAGO_ANUAL,(char *)&gastosPasivosBienesActivos->pagoTotal, 0);
     Fadd32(fml,CLI_FECHAACTUALIZACION,fechaEESSCliente,0);

     strcpy(fechaEESS,fechaEESSCliente);
   }

   strncpy(fecha,gFechaCalendario,8);

   diferenciaDias=0;

   DiferenciaFechas(fecha,fechaEESS, &diferenciaDias);

   vigenciaEESS=0;

   sts = SqlCliRecuperarDiasVigenciaEESS(&vigenciaEESS);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar los Dias de Vigencia EESS", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   if (diferenciaDias > vigenciaEESS)
   {
	strcpy(debeActualizar,"S");
   }
   else
   {
	strcpy(debeActualizar,"N");

   }

   Fadd32(fml,CLI_DEBEACTUALIZAREESS,debeActualizar, 0);
  

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Bienes Activos y Gastos Pasivos                      */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Marzo   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsActPasCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   long contador;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   int transaccionGlobal;
   char cadena[201];
   char fechaEESSCliente[8+1];
   tCliREGMOD  regModificacion;

   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(cadena, 0, sizeof(cadena));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   

   Fget32(fml, CLI_RUTX, 0, (char *)&gastosPasivosBienesActivos.rutCliente, 0);
   Fget32(fml, CLI_CODIGO_ESTADOSITUACION, 0, (char *)&gastosPasivosBienesActivos.codigo, 0);
   Fget32(fml, CLI_TIPO_ESTADOSITUACION, 0, (char *)&gastosPasivosBienesActivos.tipo, 0);
   Fget32(fml, CLI_PAGO_MENSUAL, 0, (char *)&gastosPasivosBienesActivos.pagoMensual, 0);
   Fget32(fml, CLI_PAGO_ANUAL, 0, (char *)&gastosPasivosBienesActivos.pagoTotal, 0);
   Fget32(fml, CLI_FECHAACTUALIZACION, 0, fechaEESSCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlTieneFechaActualizacionEESS(gastosPasivosBienesActivos.rutCliente,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }
   
   if (contador >0){

        sts = SqlCliModEESSFechaActualizacion(gastosPasivosBienesActivos.rutCliente,fechaEESSCliente);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }
   else
   {
	sts = SqlCliCrearEESSFechaActualizacion(gastosPasivosBienesActivos.rutCliente,fechaEESSCliente);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }


   sts = SqlCliCrearGastosPasivosBienesActivosES(gastosPasivosBienesActivos);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Activo o Gasto Pasivo ya ha sido creado", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Bien Activo o Gasto Pasivo", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaGastoPasivo(gastosPasivosBienesActivos, cadena);
      regModificacion.tipoModificacion = CLI_INGRESA_GASTOSPASIVOS;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Bienes Activos y Gastos Pasivos                  */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Marzo  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModActPasCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   char cadena[201];
   char fechaActualizacionEESS[8+1];
   char fechaEESSCliente[8+1];
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivosAnterior;
   tCliREGMOD  regModificacion;

   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&gastosPasivosBienesActivosAnterior,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(fechaActualizacionEESS, 0, sizeof(fechaActualizacionEESS));
   memset(cadena, 0, sizeof(cadena));


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&gastosPasivosBienesActivos.rutCliente, 0);
   Fget32(fml, CLI_CODIGO_ESTADOSITUACION, 0, (char *)&gastosPasivosBienesActivos.codigo, 0);
   Fget32(fml, CLI_TIPO_ESTADOSITUACION, 0, (char *)&gastosPasivosBienesActivos.tipo, 0);
   Fget32(fml, CLI_PAGO_MENSUAL, 0, (char *)&gastosPasivosBienesActivos.pagoMensual, 0);
   Fget32(fml, CLI_PAGO_ANUAL, 0, (char *)&gastosPasivosBienesActivos.pagoTotal, 0);
   Fget32(fml, CLI_FECHAACTUALIZACION, 0, fechaEESSCliente, 0);

  
   gastosPasivosBienesActivosAnterior = gastosPasivosBienesActivos;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarActivosPasivosClienteES(&gastosPasivosBienesActivosAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Parentesco.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if ((gastosPasivosBienesActivosAnterior.pagoMensual != gastosPasivosBienesActivos.pagoMensual) || (gastosPasivosBienesActivosAnterior.pagoTotal != gastosPasivosBienesActivos.pagoTotal))
   {

      rut = gastosPasivosBienesActivos.rutCliente;
      sts = SqlCliModEESSFechaActualizacion(rut,fechaEESSCliente);
      if (sts != SQL_SUCCESS)
      {
	 Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliModEESSFechaActualizacion", 0);
	 TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      sts = SqlCliModificarActivosPasivosClienteES(gastosPasivosBienesActivos);

      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Bien Activo o Gasto Pasivo", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
         /*grabar Reg. Modif.*/
         ConcatenaGastoPasivo(gastosPasivosBienesActivosAnterior, cadena);
         regModificacion.tipoModificacion = CLI_MODIFICACION_GASTOSPASIVOS;
         LlenarRegistroRegMod(fml, &regModificacion, cadena);

         sts = SqlCliCrearRegMod(regModificacion);
         if (sts != SQL_SUCCESS)
         {
            if (sts == SQL_DUPLICATE_KEY)
               Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
            else
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

            if (transaccionGlobal == 0)
               tpabort(0L);

            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
         }
         else
         {
            if (transaccionGlobal == 0)
               tpcommit(0L);

            tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
         }
      }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Registros de los Bienes Activos                   */
/* Autor   : Christian Macaya             Fecha: 09 Abril 2010          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmActPasCli(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long RutCliente;
   short codigo;
   short tipo;
   int totalRegistros=0;
   char cadena[201];
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivosAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&RutCliente, 0);
   Fget32(fml, CLI_CODIGO_ESTADOSITUACION, 0, (char *)&codigo, 0);
   Fget32(fml, CLI_TIPO_ESTADOSITUACION, 0, (char *)&tipo, 0);

   gastosPasivosBienesActivosAnterior.rutCliente = RutCliente;
   gastosPasivosBienesActivosAnterior.codigo = codigo;
   gastosPasivosBienesActivosAnterior.tipo = tipo;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarActivosPasivosClienteES(&gastosPasivosBienesActivosAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Gastos Pasivos.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarActivosPasivosClienteES(RutCliente, codigo, tipo);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar el Bienes Activos o Gasto Pasivo", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    else
   {
      /*grabar Reg. Modif.*/
      ConcatenaGastoPasivo(gastosPasivosBienesActivosAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_GASTOSPASIVOS;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }


    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/***************************************/
/* FIN GASTOS PASIVOS / BIENES ACTIVOS */
/***************************************/

/***************************************/
/* DETALLE BIENES / UBICACION INMUEBLE */
/***************************************/

/***********************************************************************/
/* Objetivo: Recupera Todos los Detalles de la Ubicacion de Inmuebles  */
/* Autor   : Christian Macaya Gatica      Fecha: 24/03/2010            */
/* Modifica:                              Fecha:                       */
/***********************************************************************/
void CliRecDetUbiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
     }

   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecDetalleBienInmuebleTodosCli(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros en Ubicación Inmuebles", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista);
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearDetalleBienInmuebleCli(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************************/
/* Objetivo: Recupera Registros del cliente en tabla DETALLEBIENESUBICACIONINMUEBLE */
/* Autor   : Christian Macaya Gatica              Fecha: 23 marzo 2010              */
/* Modifica:                                      Fecha:                            */
/************************************************************************************/
void CliRecDetUbi1ES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmueble;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&detalleUbicacionInmueble.rutCliente, 0);
   Fget32(fml, CLI_ROL_NUM_EST_SIT, 0, (char *)&detalleUbicacionInmueble.rolNum, 0);
   Fget32(fml, CLI_ROL_NUM2_EST_SIT, 0, (char *)&detalleUbicacionInmueble.rolNum2, 0);

    /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarDetalleUbicacionInmuebleES(&detalleUbicacionInmueble);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Detalle Ubicacion Inmueble" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarDetalleUbicacionInmuebleES", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUTX, (char *)&detalleUbicacionInmueble.rutCliente, 0);
   Fadd32(fml, CLI_DETALLE_BIENES_EST_SIT, detalleUbicacionInmueble.detalleBienes, 0);
   Fadd32(fml, CLI_ROL_NUM_EST_SIT, (char *)&detalleUbicacionInmueble.rolNum, 0);
   Fadd32(fml, CLI_ROL_NUM2_EST_SIT, (char *)&detalleUbicacionInmueble.rolNum2, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Detalle Ubicacion Inmueble                           */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Marzo   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDetUbiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmueble;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   int transaccionGlobal;
   long rut;
   long contador;
   char cadena[201];
   char fechaEESSCliente[8+1];
   char gFechaProceso[8+1];
   char gFechaCalendario[8+1];
   tCliREGMOD  regModificacion;


   memset((void *)&detalleUbicacionInmueble,0,sizeof(tCliDETALLEUBICACIONINMUEBLE));
   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(cadena, 0, sizeof(cadena));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroDetalleUbicacionInmuebleES(fml, &detalleUbicacionInmueble);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
       userlog("Falla recuperacion de parametros de proceso (SGT)");
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
       TRX_ABORT(transaccionGlobal);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
   
   rut = detalleUbicacionInmueble.rutCliente;
   sts = SqlTieneFechaActualizacionEESS(rut,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }

   if (contador >0){

        sts = SqlCliModEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }
   else
   {
        sts = SqlCliCrearEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        /* Se crean los registros de activos y pasivos en 0 */
        gastosPasivosBienesActivos.rutCliente = rut;
        gastosPasivosBienesActivos.codigo = 1;
        gastosPasivosBienesActivos.tipo = 1;
        gastosPasivosBienesActivos.pagoMensual = 0;
        gastosPasivosBienesActivos.pagoTotal = 0;

        sts = SqlCliCrearGastosPasivosBienesActivosES(gastosPasivosBienesActivos);

        if (sts != SQL_SUCCESS)
        {
                if (sts == SQL_DUPLICATE_KEY)
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Activo o Gasto Pasivo ya ha sido creado", 0);
                else
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Bien Activo o Gasto Pasivo", 0);

                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }

   sts = SqlCliCrearDetalleUbicacionInmuebleES(detalleUbicacionInmueble);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Detalle ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear el Detalle Ubicación Inmueble", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaUbiInmueble(detalleUbicacionInmueble, cadena);
      regModificacion.tipoModificacion = CLI_INGRESA_UBICACIONINMUEBLE;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      Fprint32(fml);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Detalle Ubicacion Inmueble                       */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Marzo  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModDetUbiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmueble;
   tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmuebleAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroDetalleUbicacionInmuebleES(fml, &detalleUbicacionInmueble);
   detalleUbicacionInmuebleAnterior = detalleUbicacionInmueble;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarDetalleUbicacionInmuebleES(&detalleUbicacionInmuebleAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Detalle Bienes/Ubicación Inmuebles.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarDetalleUbicacionInmuebleES(detalleUbicacionInmueble);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Detalle Bienes/Ubicación Inmuebles", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
    else
   {
      /*grabar Reg. Modif.*/
      ConcatenaUbiInmueble(detalleUbicacionInmuebleAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_UBICACIONINMUEBLE;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Registros de los Bienes Activos                   */
/* Autor   : Christian Macaya             Fecha: 09 Abril 2010          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmDetUbiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long RutCliente;
   long rolNum;
   long rolNum2;
   int totalRegistros=0;
   char cadena[201];
   tCliDETALLEUBICACIONINMUEBLE detalleUbicacionInmuebleAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&RutCliente, 0);
   Fget32(fml, CLI_ROL_NUM_EST_SIT, 0, (char *)&rolNum, 0);
   Fget32(fml, CLI_ROL_NUM2_EST_SIT, 0, (char *)&rolNum2, 0);

   detalleUbicacionInmuebleAnterior.rutCliente = RutCliente;
   detalleUbicacionInmuebleAnterior.rolNum = rolNum;
   detalleUbicacionInmuebleAnterior.rolNum2 = rolNum2;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarDetalleUbicacionInmuebleES(&detalleUbicacionInmuebleAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Detalle Bienes/Ubicación Inmuebles.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarDetalleUbicacionInmuebleES(RutCliente, rolNum, rolNum2);

   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar el Detalle Bienes/Ubicación Inmuebles", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
     else
   {
      /*grabar Reg. Modif.*/
      ConcatenaUbiInmueble(detalleUbicacionInmuebleAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_UBICACIONINMUEBLE;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*******************************************/
/* FIN DETALLE BIENES / UBICACION INMUEBLE */
/*******************************************/

/******************/
/* TITULO DOMINIO */
/******************/

/***********************************************************************/
/* Objetivo: Recupera Todos los Titulos Dominios de un Cliente         */
/* Autor   : Christian Macaya Gatica      Fecha: 24/03/2010            */
/* Modifica:                              Fecha:                       */
/***********************************************************************/
void CliRecTitDomES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
     }

   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecTituloDominioTodosCli(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros en Titulo Dominio", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista);
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearTituloDominioCli(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************************/
/* Objetivo: Recupera Titulo Dominio del cliente en tabla TITULODOMINIO             */
/* Autor   : Christian Macaya Gatica              Fecha: 23 marzo 2010              */
/* Modifica:                                      Fecha:                            */
/************************************************************************************/
void CliRecTitDom1ES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTITULODOMINIO tituloDominio;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&tituloDominio.rutCliente, 0);
   Fget32(fml, CLI_FOJAS_EST_SIT, 0, tituloDominio.fojas, 0);

    /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarTituloDominioES(&tituloDominio);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Titulo Dominio" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarTituloDominioES", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUTX, (char *)&tituloDominio.rutCliente, 0);
   Fadd32(fml, CLI_FOJAS_EST_SIT, tituloDominio.fojas, 0);
   Fadd32(fml, CLI_NUMERO_EST_SIT, tituloDominio.numero, 0);
   Fadd32(fml, CLI_ANO_EST_SIT, (char *)&tituloDominio.ano, 0);
   Fadd32(fml, CLI_AVALUO_FISCAL_EST_SIT, (char *)&tituloDominio.avaluoFiscal, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Titulo Dominio                                       */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Marzo   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsTitDomES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliTITULODOMINIO tituloDominio;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   int transaccionGlobal;
   long rut;
   long contador;
   char cadena[201];
   char fechaEESSCliente[8+1];
   char gFechaProceso[8+1];
   char gFechaCalendario[8+1];
   tCliREGMOD  regModificacion;

   memset((void *)&tituloDominio,0,sizeof(tCliTITULODOMINIO));
   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(cadena, 0, sizeof(cadena));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroTituloDominioES(fml, &tituloDominio);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
       userlog("Falla recuperacion de parametros de proceso (SGT)");
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
       TRX_ABORT(transaccionGlobal);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   rut = tituloDominio.rutCliente;
   sts = SqlTieneFechaActualizacionEESS(rut,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }

   if (contador >0){

        sts = SqlCliModEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }
   else
   {
        sts = SqlCliCrearEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        /* Se crean los registros de activos y pasivos en 0 */
        gastosPasivosBienesActivos.rutCliente = rut;
        gastosPasivosBienesActivos.codigo = 1;
        gastosPasivosBienesActivos.tipo = 1;
        gastosPasivosBienesActivos.pagoMensual = 0;
        gastosPasivosBienesActivos.pagoTotal = 0;

        sts = SqlCliCrearGastosPasivosBienesActivosES(gastosPasivosBienesActivos);

        if (sts != SQL_SUCCESS)
        {
                if (sts == SQL_DUPLICATE_KEY)
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Activo o Gasto Pasivo ya ha sido creado", 0);
                else
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Bien Activo o Gasto Pasivo", 0);

                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }



   sts = SqlCliCrearTituloDominioES(tituloDominio);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Titulo Dominio ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear el Titulo Dominio", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaTituloDominio(tituloDominio, cadena);
      regModificacion.tipoModificacion = CLI_INGRESA_TITULODOMINIO;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Titulo Dominio                                   */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Marzo  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModTitDomES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliTITULODOMINIO tituloDominio;
   tCliTITULODOMINIO tituloDominioAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroTituloDominioES(fml, &tituloDominio);
   tituloDominioAnterior= tituloDominio;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarTituloDominioES(&tituloDominioAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Titulo Dominio.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarTituloDominioES(tituloDominio);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Titulo Dominio", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      userlog("Entro al Historico");
      userlog("Antes del ConcatenaTituloDominio");
      ConcatenaTituloDominio(tituloDominioAnterior, cadena);
      userlog("Despues del ConcatenaTituloDominio");
      regModificacion.tipoModificacion = CLI_MODIFICACION_TITULODOMINIO;
      userlog("Antes del LlenarRegistroRegMod");
      LlenarRegistroRegMod(fml, &regModificacion, cadena);
      userlog("Despues del LlenarRegistroRegMod");

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Registros del Titulo Dominio                      */
/* Autor   : Christian Macaya             Fecha: 09 Abril 2010          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmTitDomES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliTITULODOMINIO tituloDominio;
   tCliTITULODOMINIO tituloDominioAnterior;
   int totalRegistros=0;
   char cadena[201];
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroBuscarTitDomES(fml, &tituloDominio);
   tituloDominioAnterior = tituloDominio;
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarTituloDominioES(&tituloDominioAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Titulo Dominio.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarTituloDominioES(tituloDominio);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar el Titulo Dominio", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    else
   {
      /*grabar Reg. Modif.*/
      ConcatenaTituloDominio(tituloDominioAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_TITULODOMINIO;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**********************/
/* FIN TITULO DOMINIO */
/**********************/

/************/
/* VEHICULO */
/************/

/***********************************************************************/
/* Objetivo: Recupera Todos los Vehiculos de un Cliente                */
/* Autor   : Christian Macaya Gatica      Fecha: 24/03/2010            */
/* Modifica:                              Fecha:                       */
/***********************************************************************/
void CliRecVehiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
     }

   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecVehiculosTodosCli(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros en Vehiculo", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista);
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearVehiculoCli(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*******************************************************************************/
/* Objetivo: Recupera Vehiculo del cliente en tabla VEHICULOESTSIT             */
/* Autor   : Christian Macaya Gatica              Fecha: 23 marzo 2010         */
/* Modifica:                                      Fecha:                       */
/*******************************************************************************/
void CliRecVehi1ES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliVEHICULOESTSIT vehiculoEstSit;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&vehiculoEstSit.rutCliente, 0);
   Fget32(fml, CLI_PATENTE_NUM_EST_SIT, 0, vehiculoEstSit.patenteNum, 0);

    /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarVehiculoEstSit(&vehiculoEstSit);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Vehiculo EstadoSituación" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarVehiculoEstSit", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUTX, (char *)&vehiculoEstSit.rutCliente, 0);
   Fadd32(fml, CLI_MARCA_EST_SIT, vehiculoEstSit.marca, 0);
   Fadd32(fml, CLI_MODELO_EST_SIT, vehiculoEstSit.modelo, 0);
   Fadd32(fml, CLI_ANO_EST_SIT, (char *)&vehiculoEstSit.ano, 0);
   Fadd32(fml, CLI_AVALUO_FISCAL_EST_SIT, (char *)&vehiculoEstSit.avaluoFiscal, 0);
   Fadd32(fml, CLI_PATENTE_NUM_EST_SIT, vehiculoEstSit.patenteNum, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Vehiculo en Estado de Situacion                      */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Marzo   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsVehiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliVEHICULOESTSIT vehiculoEstSit;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   int transaccionGlobal;
   long rut;
   long contador;
   char fechaEESSCliente[8+1];
   char gFechaProceso[8+1];
   char gFechaCalendario[8+1];
   char cadena[201];
   tCliREGMOD  regModificacion;

   memset((void *)&vehiculoEstSit,0,sizeof(tCliTITULODOMINIO));
   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(cadena, 0, sizeof(cadena));


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroVehiculoES(fml, &vehiculoEstSit);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
       userlog("Falla recuperacion de parametros de proceso (SGT)");
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
       TRX_ABORT(transaccionGlobal);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   rut = vehiculoEstSit.rutCliente;
   sts = SqlTieneFechaActualizacionEESS(rut,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }

   if (contador >0){

        sts = SqlCliModEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }
   else
   {
        sts = SqlCliCrearEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        /* Se crean los registros de activos y pasivos en 0 */
        gastosPasivosBienesActivos.rutCliente = rut;
        gastosPasivosBienesActivos.codigo = 1;
        gastosPasivosBienesActivos.tipo = 1;
        gastosPasivosBienesActivos.pagoMensual = 0;
        gastosPasivosBienesActivos.pagoTotal = 0;

        sts = SqlCliCrearGastosPasivosBienesActivosES(gastosPasivosBienesActivos);

        if (sts != SQL_SUCCESS)
        {
                if (sts == SQL_DUPLICATE_KEY)
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Activo o Gasto Pasivo ya ha sido creado", 0);
                else
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Bien Activo o Gasto Pasivo", 0);

                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }


   sts = SqlCliCrearVehiculoES(vehiculoEstSit);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Vehiculo ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear el Vehiculo", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaVehiculo(vehiculoEstSit, cadena);
      regModificacion.tipoModificacion = CLI_INGRESA_VEHICULOS;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Vehiculo en Estado de Situacion                  */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Marzo  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModVehiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliVEHICULOESTSIT vehiculoEstSit;
   tCliVEHICULOESTSIT vehiculoEstSitAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroVehiculoES(fml, &vehiculoEstSit);
   vehiculoEstSitAnterior= vehiculoEstSit;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarVehiculoEstSit(&vehiculoEstSitAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Vehiculo.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarVehiculoES(vehiculoEstSit);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Vehiculo", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaVehiculo(vehiculoEstSitAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_VEHICULOS;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Eliminar Registros del Vehiculo en Estado de Situacion     */
/* Autor   : Christian Macaya             Fecha: 09 Abril 2010          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliElmVehiES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliVEHICULOESTSIT vehiculoEstSit;
   tCliVEHICULOESTSIT vehiculoEstSitAnterior;
   int totalRegistros=0;
   char cadena[201];
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroBuscarVehiculoES(fml, &vehiculoEstSit);
   vehiculoEstSitAnterior = vehiculoEstSit;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarVehiculoEstSit(&vehiculoEstSitAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar Vehiculo.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarVehiculoES(vehiculoEstSit);

   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar el Vehiculo", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    else
   {
      /*grabar Reg. Modif.*/
      ConcatenaVehiculo(vehiculoEstSitAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_VEHICULOS;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************/
/* FIN VEHICULO */
/****************/

/************************/
/* BIEN ACTIVO ACREEDOR */
/************************/

/***********************************************************************/
/* Objetivo: Recupera Todos los Bienes Acreedores de un Cliente        */
/* Autor   : Christian Macaya Gatica      Fecha: 24/03/2010            */
/* Modifica:                              Fecha:                       */
/***********************************************************************/
void CliRecBienAcES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long rut;
   Q_HANDLE *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
     if (tpbegin(TIME_OUT,0L) == -1)
     {
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
       tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
     }

   Fget32(fml,CLI_RUTX,0,(char *)&rut,0);

   if ((lista = QNew()) != NULL)
      sts = SqlRecBienAcreedorTodosCli(lista, rut);
   else
      sts = SQL_MEMORY;

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml,HRM_MENSAJE_SERVICIO,"Error al recuperar registros en Bien Acreedor", 0);

      if (transaccionGlobal == 0)
         tpabort(0L);
      QDelete(lista);
      tpreturn(TPFAIL, 0, (char *)fml, 0L, 0L);
   }

   FMLSetearBienAcreedorCli(fml, lista);

   if (transaccionGlobal == 0)
      tpcommit(0L);

   QDelete(lista);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/******************************************************************************************/
/* Objetivo: Recupera Bien Activo Acreedor del cliente en tabla BIENESACTIVOSACREEDORES   */
/* Autor   : Christian Macaya Gatica              Fecha: 23 marzo 2010                    */
/* Modifica:                                      Fecha:                                  */
/******************************************************************************************/
void CliRecBienAc1ES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliBIENACTIVOACRE bienAcreedor;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&bienAcreedor.rutCliente, 0);
   Fget32(fml, CLI_RUT, 0, (char *)&bienAcreedor.rut, 0);

    /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarBienAcreedorEstSit(&bienAcreedor);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Usuario indicado no existe en Bien Acreedor - EstadoSituación" , 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliRecuperarBienAcreedorEstSit", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_RUTX, (char *)&bienAcreedor.rutCliente, 0);
   Fadd32(fml, CLI_APELLIDOPATERNO, bienAcreedor.apellidoPaterno, 0);
   Fadd32(fml, CLI_APELLIDOMATERNO, bienAcreedor.apellidoMaterno, 0);
   Fadd32(fml, CLI_NOMBRE, bienAcreedor.nombres, 0);
   Fadd32(fml, CLI_RUT, (char *)&bienAcreedor.rut, 0);
   Fadd32(fml, CLI_DIGVER, bienAcreedor.dv, 0);
   Fadd32(fml, CLI_INTACREE_EST_SIT, bienAcreedor.intAcreedor, 0);
   Fadd32(fml, CLI_MONTODEUDA_EST_SIT, (char *)&bienAcreedor.montodeuda, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Bien Acreedor en Estado de Situacion                 */
/* Autor   : Christian Macaya Gatica      Fecha:  23  Marzo   2010      */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsBienAcES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliBIENACTIVOACRE bienAcreedor;
   tCliGASTOSPASIVOSBIENESACTIVOS gastosPasivosBienesActivos;
   int transaccionGlobal;
   long rut;
   long contador;
   char cadena[201];
   char fechaEESSCliente[8+1];
   char gFechaProceso[8+1];
   char gFechaCalendario[8+1];
   tCliREGMOD  regModificacion;

   memset((void *)&bienAcreedor,0,sizeof(tCliBIENACTIVOACRE));
   memset((void *)&gastosPasivosBienesActivos,0,sizeof(tCliGASTOSPASIVOSBIENESACTIVOS));
   memset((void *)&regModificacion,0,sizeof(tCliREGMOD));
   memset(fechaEESSCliente, 0, sizeof(fechaEESSCliente));
   memset(cadena, 0, sizeof(cadena));


   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroBienAcreedorES(fml, &bienAcreedor);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
       userlog("Falla recuperacion de parametros de proceso (SGT)");
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
       TRX_ABORT(transaccionGlobal);
       tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   rut = bienAcreedor.rutCliente;
   sts = SqlTieneFechaActualizacionEESS(rut,&contador);
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlTieneFechaActualizacionEESS", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);

   }

   if (contador >0){

        sts = SqlCliModEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }
   else
   {
        sts = SqlCliCrearEESSFechaActualizacion(rut,gFechaCalendario);
        if (sts != SQL_SUCCESS)
        {
                Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliCrearEESSFechaActualizacion", 0);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }

        /* Se crean los registros de activos y pasivos en 0 */
        gastosPasivosBienesActivos.rutCliente = rut;
        gastosPasivosBienesActivos.codigo = 1;
        gastosPasivosBienesActivos.tipo = 1;
        gastosPasivosBienesActivos.pagoMensual = 0;
        gastosPasivosBienesActivos.pagoTotal = 0;

        sts = SqlCliCrearGastosPasivosBienesActivosES(gastosPasivosBienesActivos);

        if (sts != SQL_SUCCESS)
        {
                if (sts == SQL_DUPLICATE_KEY)
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Activo o Gasto Pasivo ya ha sido creado", 0);
                else
                        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Bien Activo o Gasto Pasivo", 0);

                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }

   sts = SqlCliCrearBienAcreedorES(bienAcreedor);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "El Bien Acreedor ya ha sido creada", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear el Bien Acreedor", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaBienAcreedor(bienAcreedor, cadena);
      regModificacion.tipoModificacion = CLI_INGRESA_BIENACREEDOR;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Modificar Bien Acreedor en Estado de Situacion             */
/* Autor   : Christian Macaya Gatica      Fecha: 23  Marzo  2010        */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModBienAcES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char cadena[201];
   tCliBIENACTIVOACRE bienAcreedor;
   tCliBIENACTIVOACRE bienAcreedorAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   LlenarRegistroBienAcreedorES(fml, &bienAcreedor);
   bienAcreedorAnterior = bienAcreedor;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarBienAcreedorEstSit(&bienAcreedorAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar el Bien Acreedor.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliModificarBienAcreedorES(bienAcreedor);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe el Bien Acreedor", 0);
      else
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   else
   {
      /*grabar Reg. Modif.*/
      ConcatenaBienAcreedor(bienAcreedorAnterior, cadena);
      regModificacion.tipoModificacion = CLI_MODIFICACION_BIENACREEDOR;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************/
/* Objetivo: Eliminar Registros del Bien Acreedor en Estado de Situacion  */
/* Autor   : Christian Macaya             Fecha: 09 Abril 2010            */
/* Modifica:                              Fecha:                          */
/**************************************************************************/
void CliElmBienAcES(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long RutCliente;
   long rut;
   int totalRegistros=0;
   char cadena[201];
   tCliBIENACTIVOACRE bienAcreedorAnterior;
   tCliREGMOD  regModificacion;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&RutCliente, 0);
   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);

   bienAcreedorAnterior.rutCliente = RutCliente;
   bienAcreedorAnterior.rut = rut;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarBienAcreedorEstSit(&bienAcreedorAnterior);

   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Recuperar el Bien Acreedor.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   sts = SqlCliEliminarBienAcreedorES(RutCliente, rut);
   if ((sts != SQL_SUCCESS) && (sts != SQL_NOT_FOUND))
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar el Bien Acreedor", 0);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    else
   {
      /*grabar Reg. Modif.*/
      ConcatenaBienAcreedor(bienAcreedorAnterior, cadena);
      regModificacion.tipoModificacion = CLI_ELIMINACION_BIENACREEDOR;
      LlenarRegistroRegMod(fml, &regModificacion, cadena);

      sts = SqlCliCrearRegMod(regModificacion);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO,"El Reg. Mod. ya ha sido creado",0);
         else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Reg. Mod.", 0);

         if (transaccionGlobal == 0)
            tpabort(0L);

         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }
      else
      {
      if (transaccionGlobal == 0)
         tpcommit(0L);

      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
       }
   }

    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************************/
/* FIN BIEN ACTIVO ACREEDOR */
/****************************/


/************************************************************************/
/* INICIO: Modificación Relacion (TipoEmpleo/Actividad/TipoRenta)       */
/************************************************************************/

/*************************************************************************/
/* Objetivo: Recuperar lista de Relaciones Actividad/Tipos de Renta      */
/* Autor   : Christian Macaya Gatica                  Fecha: 29-09-2010  */
/*************************************************************************/
void CliLisACTTipRen(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliACTTIPORENTA      *actividadtipoRenta;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarActividadTipoRentaTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No hay Relaciones Actividad/Tipos de Renta", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar las Relaciones Actividad/Tipos de Renta", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3 * totalRegistros,
          sizeof(tCliACTTIPORENTA)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      actividadtipoRenta = (tCliACTTIPORENTA  *) QGetItem( lista, i);

      Fadd32(fml, CLI_TIPO_RENTA_CODIGO,        (char *)&actividadtipoRenta->codigoTipoRenta, i);
      Fadd32(fml, CLI_ACTIVIDAD_CODIGO,  	(char *)&actividadtipoRenta->codigoActividad, i);
      Fadd32(fml, CLI_TIPO_RENTA_DESCRIPCION,   actividadtipoRenta->descripcion , i); 
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* FIN: Modificación Relacion (TipoEmpleo/Actividad/TipoRenta)          */
/************************************************************************/

/*************************************************************************/
/* Objetivo: Recupera todos los tipos de renta                           */
/* Autor   : Cristian Osorio Iribarren                Fecha: 05-10-2010  */
/*************************************************************************/
void CliRecTodTiRe(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   Q_HANDLE              *lista;
   int                   i=0;
   tCliTIPORENTA         *tipoRenta;
   int totalRegistros=0;

  /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Error grave.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarTipoRentaTodos(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No hay tipos de renta", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar los tipos de renta", 0);

      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros,
          sizeof(tCliTIPORENTA)* totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      tipoRenta = (tCliTIPORENTA  *) QGetItem( lista, i);

      Fadd32(fml, CLI_TIPO_RENTA_CODIGO,        (char *)&tipoRenta->codigo, i);
      Fadd32(fml, CLI_TIPO_RENTA_DESCRIPCION,   tipoRenta->descripcion , i);
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/****************************************/
/* INICIO: Nuevo Campo Parametro EESS   */
/****************************************/

/*************************************************************************/
/* Objetivo: Recuperar nuevo parametro EESS                              */
/* Autor   : Christian Macaya Gatica                  Fecha: 12-10-2010  */
/*************************************************************************/
void CliRecVigenEESS(TPSVCINFO *rqst)
{
   FBFR32                *fml;
   int                   transaccionGlobal;
   long                  sts;
   int                   i=0;
   short                 vigenciaEESS=0;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarDiasVigenciaEESS(&vigenciaEESS);
   if (sts != SQL_SUCCESS)
   {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "cliente - ERROR: No se logro recuperar los Dias de Vigencia EESS", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   Fadd32(fml, CLI_VIGENCIA_EESS,        (char *)&vigenciaEESS, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/*************************************/
/* FIN: Nuevo Campo Parametro EESS   */
/*************************************/

void CliInsLogBcaTel(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal=0;
   long sts=0;
   char fechaProceso[8+1];

   tCliLOGBANCATELEFONICA logBancaTelefonica;

   /****** Buffer de entrada ******/
   memset((void *)&logBancaTelefonica, 0, sizeof(tCliLOGBANCATELEFONICA));

   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&logBancaTelefonica.rut, 0);
   Fget32(fml, CLI_NUMERO, 0, logBancaTelefonica.celular, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&logBancaTelefonica.ejecutivo, 0);
   Fget32(fml, CLI_BIEN_TIPO, 0, (char *)&logBancaTelefonica.tipoValidacion, 0);
   Fget32(fml, CLI_SUCURSAL, 0, (char *)&logBancaTelefonica.sucursal, 0);
   Fget32(fml, CLI_ESTADO, 0, (char *)&logBancaTelefonica.estadoClave, 0);
   Fget32(fml, CLI_DESCRIPCION_ESTADO, 0, logBancaTelefonica.estadoEvento, 0);

   /****** Cuerpo del servicio ******/

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   strcpy(fechaProceso, gFechaProceso);
   strcpy(logBancaTelefonica.fechaProceso, fechaProceso);

   sts = SqlCliInsertarLogBancaTelefonica(logBancaTelefonica);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error al insertar log banca telefonica", 0);
      userlog("Error al insertar log banca telefonica");
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliInsCtaOtrBan (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliCtaOtroBanco cuentaOtroBanco;
   long sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUT, 0, (char *)&cuentaOtroBanco.rutCliente, 0);
   Fget32(fml, CLI_BANCO, 0, (char *)&cuentaOtroBanco.codigoBanco, 0);
   Fget32(fml, CLI_CUENTA, 0, cuentaOtroBanco.numeroCuenta, 0);
   Fget32(fml, CLI_TIPO_CUENTA, 0, (char *)&cuentaOtroBanco.codigoTipoCuenta, 0);

   sts = SqlCliInsCtaOtrBan(&cuentaOtroBanco);
   if (sts != SQL_SUCCESS || sts == SQL_NOT_FOUND)
   {
      if (sts == SQL_DUPLICATE_KEY)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "La cuenta del cliente ya existe o ha sido creada", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar cuenta otro banco", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecCtaOtrBan (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   Q_HANDLE *lista;
   int transaccionGlobal;
   tCliCtaOtroBanco *cuentasOtrosBancos;
   long rutCliente = 0;
   long sts;
   int i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   Fget32(fml, CLI_RUT, 0, (char *)&rutCliente, 0);

   sts = SqlCliRecCtaOtrBan(&rutCliente, lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar cuenta otro banco, no existen registros de otras cuentas para este cliente.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar cuenta otro banco", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)3, (short)0, (short)3, (long)50);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i = 0; i < lista->count; i++)
   {
      cuentasOtrosBancos = (tCliCtaOtroBanco *) QGetItem(lista, i);
      Fadd32(fml, CLI_BANCO, (char *)&cuentasOtrosBancos->codigoBanco, i);
      Fadd32(fml, CLI_CUENTA, (char *)&cuentasOtrosBancos->numeroCuenta, i);
      Fadd32(fml, CLI_TIPO_CUENTA, (char *)&cuentasOtrosBancos->codigoTipoCuenta, i);
      Fadd32(fml, CLI_NOMBRE_BANCO, (char *)&cuentasOtrosBancos->nombreBanco, i);
      Fadd32(fml, CLI_DESCRIPCION_CUENTA, (char *)&cuentasOtrosBancos->descripcionCuenta, i);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/* ACD */

void CliInsExtCorreo(TPSVCINFO *rqst) 
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;
        long sts=0;  /* Modificaciones ACD */

	tCliEXTCORREO extensionCorreo;
	tCliEXTCORREO *extensionCorreoAnt; /* Modificaciones ACD */

	memset((void *)&extensionCorreo, 0,
			sizeof(tCliEXTCORREO));
	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

        /* Modificaciones ACD */
	Fget32(fml, CLI_EXT_CORREO_EXTENSION  , 0, extensionCorreo.extension, 0);
	Fget32(fml, CLI_EXT_CORREO_DESCRIPCION, 0, extensionCorreo.descripcion, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

        strcpy(extensionCorreoAnt->extension, extensionCorreo.extension);

        sts=SqlCliRecuperarUnaExtensionesCorreo(extensionCorreoAnt);

        if (sts == SQL_NOT_FOUND )
        {
           lngSts = SqlCliInsertarExtensionCorreo(extensionCorreo);

           if (lngSts != SQL_SUCCESS)
           {
                   if (lngSts == SQL_DUPLICATE_KEY)
                   {
                           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Extension de correo ya se encuentra definida", 0);
                   }
                   else
                   {
                           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al crear Extension de correo  ", 0);
                   }
                   TRX_ABORT(intTransaccionGlobal);
                   tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
           }
           TRX_COMMIT(intTransaccionGlobal);
           tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
        }
        else
        {
           if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND )
           {
                 Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
                 printf("Error al recuperar registros [%s] " , extensionCorreo.extension);
                 TRX_ABORT(intTransaccionGlobal);
                 tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
           } else {
                 Fadd32(fml, HRM_MENSAJE_SERVICIO, "Extension de Correo Electronico Existente ", 0);
                 printf("Extension de Correo Electronico Existente [%s] " , extensionCorreo.extension);
                 TRX_ABORT(intTransaccionGlobal);
                 tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
           }
        }

	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliEliExtCorreo(TPSVCINFO *rqst) 
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliEXTCORREO extensionCorreo;

	memset((void *)&extensionCorreo, 0,
			sizeof(tCliEXTCORREO));

	/******   Buffer de entrada   ******/
	fml = (FBFR32 *)rqst->data;

        /* Modificaciones ACD */
	Fget32(fml, CLI_EXT_CORREO_EXTENSION, 0, extensionCorreo.extension, 0);

	/******   Cuerpo del servicio   ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliEliminarExtensionCorreo(&extensionCorreo);

	if (lngSts != SQL_SUCCESS) 
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar Extension de Correo", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliModExtCorreo(TPSVCINFO *rqst) 
{
	FBFR32 *fml;
	int intTransaccionGlobal=0;
	long lngSts=0;

	tCliEXTCORREO extensionCorreo;

	/****** Buffer de entrada ******/
	memset((void *)&extensionCorreo, 0, sizeof(tCliEXTCORREO));
	fml = (FBFR32 *)rqst->data;

        /* Modificaciones ACD */
	Fget32(fml, CLI_EXT_CORREO_EXTENSION, 0, extensionCorreo.extension, 0);
	Fget32(fml, CLI_EXT_CORREO_DESCRIPCION, 0, extensionCorreo.descripcion, 0);

	/****** Cuerpo del servicio ******/

	intTransaccionGlobal = tpgetlev();

	TRX_BEGIN(intTransaccionGlobal);

	lngSts = SqlCliModificarExtensionCorreo(&extensionCorreo);

	if (lngSts != SQL_SUCCESS) 
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar Extension de Correo ", 0);
		TRX_ABORT(intTransaccionGlobal);
		tpreturn(TPFAIL, ErrorServicio(lngSts, rqst->name), (char *)fml, 0L, 0L);
	}
	TRX_COMMIT(intTransaccionGlobal);
	tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

void CliRecExtCorreo(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel;
    Q_HANDLE             *listaExtensionCorreo=NULL;
    tCliEXTCORREO        *extensionCorreo=NULL;	
    int                  i=0;
    long                 sts=0;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
 
    /* Modificaciones ACD */
    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);

    if((listaExtensionCorreo = (Q_HANDLE *) QNew()) == NULL)
    {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(SQL_MEMORY, rqst->name), (char *)fml, 0L, 0L);
    }

    sts = SqlCliRecuperarExtensionesCorreo(listaExtensionCorreo);
    if (sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen extensiones de correo electronico ingresadas", 0);
        }
        else
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar Extensiones de Correo Electronico", 0);
        }
        QDelete(listaExtensionCorreo);
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
    }

    tpfree((char *)fml);
    fml = NewFml32((long)listaExtensionCorreo->count, (short)1, (short)0, (short)2, (long)200);

    for(i=0; i<listaExtensionCorreo->count; i++)
    {
        extensionCorreo  = (tCliEXTCORREO *) QGetItem(listaExtensionCorreo, i);
        Fadd32(fml, CLI_EXT_CORREO_CODIGO     , (char *)&extensionCorreo->codigo     ,i);
        Fadd32(fml, CLI_EXT_CORREO_EXTENSION  ,          extensionCorreo->extension  ,i);
        Fadd32(fml, CLI_EXT_CORREO_DESCRIPCION,          extensionCorreo->descripcion,i);
    }

    QDelete(listaExtensionCorreo);
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecUnaExtCor(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   int totalRegistros=0, i, largoRetorno;
   char extension[6];
   
   tCliEXTCORREO *extensionCorreo;
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   
   Fget32(fml, CLI_EXT_CORREO_EXTENSION, 0,extension, 0);
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT,0L) == -1)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 

  extensionCorreo = (tCliEXTCORREO *)calloc(CLI_MAX_EXT_CORREO, sizeof(tCliEXTCORREO));
  if (extensionCorreo == NULL)
  {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error inesperado en la Aplicacion", 0);
      if (transaccionGlobal == 0)
         tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
  }
  
  totalRegistros = 1;
  strcpy(extensionCorreo->extension,extension);
  
  sts=SqlCliRecuperarUnaExtensionesCorreo(extensionCorreo);

  if (sts != SQL_SUCCESS)
  {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen extensiones de correo", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar registros", 0);
 
      free(extensionCorreo);
      if (transaccionGlobal == 0)
         tpabort(0L);
 
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
  }
 
   fml = NewFml32((long)totalRegistros, (short)1, (short)0, (short)2, (long)100);
 
   ObtenerRegistroExtension(fml, extensionCorreo, totalRegistros);
   free(extensionCorreo);
 
   if (transaccionGlobal == 0)
      tpcommit(0L);
 
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecActLabEst(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel;
    Q_HANDLE             *listaActLabESt=NULL;
    tCarACTLABEST        *actlabest=NULL;
    int                  i=0;
    long                 sts=0;

    /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);

    if((listaActLabESt = (Q_HANDLE *) QNew()) == NULL)
    {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar memoria." , 0);
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(SQL_MEMORY, rqst->name), (char *)fml, 0L, 0L);
    }

    sts = SqlCliRecuperarActLabEst(listaActLabESt);
    if (sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No hay actividad laboral estudiante ingresadas", 0);
        }
        else
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar actividad laboral estudiante", 0);
        }
        QDelete(listaActLabESt);
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
    }

    tpfree((char *)fml);
    fml = NewFml32((long)listaActLabESt->count, (short)1, (short)0, (short)1, (long)512);

    for(i=0; i<listaActLabESt->count; i++)
    {
        actlabest  = (tCarACTLABEST *) QGetItem(listaActLabESt, i);
        Fadd32(fml, CLI_RUT,(char *)&actlabest->rutcliente, i);
		Fadd32(fml, CLI_CARRERA,(char *)&actlabest->codigocarrera, i);
		Fadd32(fml, CLI_UNIVERSIDAD,(char *)&actlabest->codigouniversidad, i);
		Fadd32(fml, CLI_ANO_INGRESO,(char *)&actlabest->anoestudio, i);
		Fadd32(fml, CLI_TIPO_CARRERA,(char *)&actlabest->codigotipocarrera, i);
    }

    QDelete(listaActLabESt);
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliInsActLabEst(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel=0;
    tCarACTLABEST        actlabest;
    long                 sts=0;

    memset((char *)&actlabest, (int)0, sizeof(tCarACTLABEST));
    
    /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);

    Fget32(fml, CLI_RUT, 0, (char *)&actlabest.rutcliente, 0);
    Fget32(fml, CLI_CARRERA, 0, (char *)&actlabest.codigocarrera, 0);
    Fget32(fml, CLI_UNIVERSIDAD, 0,(char *)&actlabest.codigouniversidad, 0);
    Fget32(fml, CLI_ANO_INGRESO, 0,(char *)&actlabest.anoestudio, 0);
	Fget32(fml, CLI_TIPO_CARRERA, 0,(char *)&actlabest.codigotipocarrera, 0);

    sts = SqlCliInsertarActLabEst(&actlabest);
    if(sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una actividad laboral estudiante para ese rut por lo tanto no se puede insertar." , 0);
        }
        else if(sts == SQL_DUPLICATE_KEY)
        {
        /* llamar al mod */
           sts = SqlCliModificarActLabEst(&actlabest);
           if(sts != SQL_SUCCESS)
           {
               if (sts == SQL_NOT_FOUND)
                {
                  Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una actividad laboral estudiante para ese rut por lo tanto no se puede modificar." , 0);
                }
               else
                {
                  Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar la actividad laboral estudiante." , 0);
                }
               TRX_ABORT(nivel);
               tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
            }
        }
        else
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar la actividad laboral estudiante." , 0);
        }
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliModActLabEst(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel=0;
    tCarACTLABEST        actlabest;
    long                 sts=0;

    memset((char *)&actlabest, 0, sizeof(tCarACTLABEST));
    
    /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);

    Fget32(fml, CLI_RUT, 0, (char *)&actlabest.rutcliente, 0);
	Fget32(fml, CLI_CARRERA, 0, (char *)&actlabest.codigocarrera, 0);
    Fget32(fml, CLI_UNIVERSIDAD, 0,(char *)&actlabest.codigouniversidad, 0);
	Fget32(fml, CLI_ANO_INGRESO, 0,(char *)&actlabest.anoestudio, 0);
	Fget32(fml, CLI_TIPO_CARRERA, 0,(char *)&actlabest.codigotipocarrera, 0);

    sts = SqlCliModificarActLabEst(&actlabest);
    if(sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una actividad laboral estudiante para ese rut por lo tanto no se puede modificar." , 0);
        }
        else
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar la actividad laboral estudiante." , 0);
        }
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliEliActLabEst(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel=0;
    tCarACTLABEST        actlabest;
    long                 sts=0;

    memset((char *)&actlabest, 0, sizeof(tCarACTLABEST));
    
    /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);

    Fget32(fml, CLI_RUT, 0, (char *)&actlabest.rutcliente, 0);

    sts = SqlCliEliminarActLabEst(&actlabest);
    if(sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una actividad laboral estudiante para ese rut por lo tanto no se puede eliminar." , 0);
        }
        else
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar la actividad laboral estudiante." , 0);
        }
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
    }
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

void CliRecUnActLabE(TPSVCINFO *rqst)
{
    FBFR32               *fml=NULL;
    int                  nivel=0;
    long                 sts=0;
    tCarACTLABEST        actlabest;
	
	long                 rut;

    memset((char *)&actlabest, 0, sizeof(tCarACTLABEST));

    /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    /******   Cuerpo del servicio   ******/
    nivel = tpgetlev();
    TRX_BEGIN(nivel);
	
	if( Foccur32( fml, CLI_RUT) )
      Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);
	else
	{
		Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente - Falló al recuperar el Rut ingresado.", 0);
		TRX_ABORT(nivel);
		tpreturn(TPFAIL,ErrorServicio(sts, rqst->name), (char *) fml, 0L, 0L);
	}

    sts = SqlCliRecuperarUnaActLabEst(&actlabest,rut);
    if (sts != SQL_SUCCESS)
    {
        if (sts == SQL_NOT_FOUND)
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe una actividad laboral estidiante para ese rut.", 0);
        }
        else
        {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar actividad laboral estudiante.", 0);
        }
        TRX_ABORT(nivel);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
    }

	Fadd32(fml, CLI_RUT,(char *)&actlabest.rutcliente, 0);
	Fadd32(fml, CLI_CARRERA,(char *)&actlabest.codigocarrera, 0);
	Fadd32(fml, CLI_UNIVERSIDAD,(char *)&actlabest.codigouniversidad, 0);
	Fadd32(fml, CLI_ANO_INGRESO,(char *)&actlabest.anoestudio, 0);
	Fadd32(fml, CLI_TIPO_CARRERA,(char *)&actlabest.codigotipocarrera, 0);
	
    TRX_COMMIT(nivel);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/* ACD */

/******************************************************************************/
/* Objetivo: Modifica cliente en tabla RCLIENTESECTORSII                       */
/* Autor   : Susana Gonzalez                      Fecha: 30 abril 2010        */
/* Modifica:                                      Fecha:                      */
/******************************************************************************/
void CliModCorreo(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCLIENTE cliente;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUT, 0, (char *)&cliente.rut, 0);
   Fget32(fml, CLI_EMAIL, 0, cliente.direccionElectronica, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliModificarClienteMail(cliente);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error en SqlCliModificarClienteMail", 0);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Inserta operaciones a las cuales se le asigno aumento de cupo automatico */
/* Tablas  : OperacionAumentoCupo                                                     */
/* Autor   : Andres Gonzalez              Fecha:  Junio   2012                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliAsigCupAut (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   tCliOperacionAmentoCupo operacionAumentoCupo;
   char operacion[30];
   long sts;
   int i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   for (i=0; i<Foccur32(fml,CLI_RUT); i++){
      Fget32(fml, CLI_RUT, i, (char *)&operacionAumentoCupo.rut, 0);
      Fget32(fml, CLI_NUMERO, i, operacion, 0);
      Fget32(fml, CLI_CODIGO_SISTEMA, i, (char *)&operacionAumentoCupo.sistema, 0);
      Fget32(fml, CLI_DESCRIPCION, i, operacionAumentoCupo.esLineaSobreGiro, 0);

      userlog("CLIENTE CUSTODIA rut:[%li]",operacionAumentoCupo.rut);
      userlog("CLIENTE CUSTODIA ope:[%s]",operacion);
      userlog("CLIENTE CUSTODIA sis:[%d]",operacionAumentoCupo.sistema);
      userlog("CLIENTE CUSTODIA desc:[%s]",operacionAumentoCupo.esLineaSobreGiro);

      if (strlen(operacion) > 16) operacion[16]=0;

      strcpy(operacionAumentoCupo.operacion,operacion);

      userlog("CLIENTE CUSTODIA ope:[%s]",operacionAumentoCupo.operacion);

      sts = SqlCliExisteOperacionAumentoCupo(operacionAumentoCupo.rut,operacionAumentoCupo.operacion);
      if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar operacion aumento cupo automatico", 0);

         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
      }

      if (sts == SQL_NOT_FOUND){
        sts = SqlCliInsertarOperacionAumentoCupo(&operacionAumentoCupo);
        if (sts != SQL_SUCCESS)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar operacion aumento cupo automatico", 0);

           TRX_ABORT(transaccionGlobal);
           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
      }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Elimina operaciones a las cuales se le asigno aumento de cupo automatico */
/* Tablas  : OperacionAumentoCupo                                                     */
/* Autor   : Andres Gonzalez              Fecha:  Junio   2012                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliElmCupAut (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long rut;
   long sts;
   int i;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);


   sts = SqlCliExistenOpeAuCupoXRut(rut);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar operacion aumento cupo automatico", 0);

         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (sts == SQL_SUCCESS){
        sts = SqlCliEliminarOperacionAumentoCupo(rut);
        if (sts != SQL_SUCCESS)
        {
           Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al eliminar operacion aumento cupo automatico", 0);

           TRX_ABORT(transaccionGlobal);
           tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
        }
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Existen operaciones a las cuales se le asigno aumento de cupo automatico */
/* Tablas  : OperacionAumentoCupo                                                     */
/* Autor   : Andres Gonzalez              Fecha:  Junio   2012                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliExisteCupAut (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long rut;
   long sts;
   int i;
   long existe=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUT, 0, (char *)&rut, 0);


   sts = SqlCliExistenOpeAuCupoXRut(rut);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar operacion aumento cupo automatico", 0);

         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if (sts == SQL_NOT_FOUND)
      existe=0;
   else
      existe=1;

   Fadd32(fml, CLI_EXISTE_CLIENTE, (char *)&existe, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/* + Otorgamiento Clave */
/************************************************************************/
/* Objetivo: Crear syslog                                               */
/* Autor   : Dinorah Painepan               Fecha:  Marzo 2013          */
/* Modificado   : Alex Rojas                Fecha:  Septiembre 2014     */
/************************************************************************/
void CliSyslog(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   int transaccionGlobal;
   char   fechaCalendario[DIM_FECHA_HORA];
   time_t hora;
   tHrmBufferTrans  bufferSysLog;
   tCliOtorgamientoClave cliOtorgamientoClave;
   short datosDeEntradaValidos = 1;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);
   
   if (strcmp(rqst->name, "CliSysOtorClave") == 0)
   {
      if (!Foccur32(fml, CLI_RUT))
         datosDeEntradaValidos = 0;
      Fget32(fml, CLI_RUT, 0, (char *)&cliOtorgamientoClave.rutCliente, 0);

      if (!Foccur32(fml, CLI_USUARIO))
         datosDeEntradaValidos = 0;
      Fget32(fml, CLI_USUARIO, 0, (char *)&cliOtorgamientoClave.usuario, 0);

      if (!Foccur32(fml, CLI_SUCURSAL))
         datosDeEntradaValidos = 0;
      Fget32(fml, CLI_SUCURSAL, 0, (char *)&cliOtorgamientoClave.sucursal, 0);

      if (!Foccur32(fml, CLI_FECHA_CREACION))
         datosDeEntradaValidos = 0;
      Fget32(fml, CLI_FECHA_CREACION, 0, cliOtorgamientoClave.fechaHora, 0);

      if (!Foccur32(fml, CLI_TIPO_MODIFICACION))
         datosDeEntradaValidos = 0;
      Fget32(fml, CLI_TIPO_MODIFICACION, 0, (char *)&cliOtorgamientoClave.tipoCambioClave, 0);

      Fget32(fml, CLI_DESCRIPCION, 0, cliOtorgamientoClave.ip, 0);
      
      if (!datosDeEntradaValidos)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Datos de entrada incompletos", 0);
         userlog("Datos de entrada incompletos");
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
    
      sts = SqlCliInsertarOtorgamientoClave(&cliOtorgamientoClave);
      if (sts != SQL_SUCCESS)
      {
         if (sts == SQL_DUPLICATE_KEY)
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Ya existe un registro con la combinacion (rut cliente - usuario - fecha hora)", 0);
         else
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo insertar el registro en la tabla OtorgamientoClave", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   else
   {
      memset((char *)&bufferSysLog, 0, sizeof(tHrmBufferTrans));
      Fget32(fml, CLI_DESCRIPCION, 0, bufferSysLog.mensajeDetalle, 0);

      bufferSysLog.formatoLibre = CLI_FORMATO_EXCLUIDO;
      hora = (time_t)time((time_t *)NULL);
      strftime(fechaCalendario, DIM_FECHA_HORA, "%d%m%Y%H%M%S", localtime(&hora));
      strcpy(bufferSysLog.fechaHora,fechaCalendario);
      OmEscribirEnSysLog(&bufferSysLog);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/* - Otorgamiento Clave */

/* Implantación de Normativa FATCA PJVA INI */
/**************************************************************************************/
/* Objetivo: Valida el cliente es potencial US Person                                 */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliValCliFATCA (TPSVCINFO *rqst)
{
   FBFR32           *fml;
   int              transaccionGlobal;
   tCliCLIENTE      cliente;
   short            esPontecialUsPerson = 0;
   long             sinPaisNacionalidad = 0;
   long             sinPaisResidencia = 0;
   long             sinPaisNacimiento = 0;   
   long             sts;
   char             msjCamposEnBlanco[250+1];
   char             msjEsPotencialUsPerson[250+1];
   char             mensaje[250+1];   
   short            existeClienteFatca = 0;
   short            permiteValidar = 0;
   char             fechaProceso[8 + 1];   
   tCliClienteFATCA clienteFatca;
   Q_HANDLE         *lista;
   

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((void *)&cliente,0,sizeof(tCliCLIENTE));
   memset((char *)&clienteFatca,0,sizeof(tCliClienteFATCA));

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUTX, 0, (char *)&cliente.rut, 0);
   
   clienteFatca.rut = cliente.rut;

   if (cliente.rut >= 50000000){
      strcpy(mensaje, "");
	  esPontecialUsPerson = 0;

      Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
      Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
	  Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);

      TRX_COMMIT(transaccionGlobal);
      tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);	  
   }
   
   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }
   
   /********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
   /********* ContinuidadHoraria [FIN] *********/

   strcpy(fechaProceso, gFechaProceso);

   if (strcmp(rqst->name, "CliValCliFicha") == 0)
   {
      sts = SqlCliRecuperarOperacionesProductoFatca(clienteFatca.rut, lista);
	  if(sts != SQL_SUCCESS)
	  {
	     if (sts == SQL_NOT_FOUND)
		 {
		    permiteValidar = 0;
			Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clasificacion de cliente no existe.", 0);
		 }
		 else
		 {
		    Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA.", 0);      
			TRX_ABORT(transaccionGlobal);
			tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		 }
	  }
	  else
	  {
	     permiteValidar = 1;
	  }
   }
   else
   {
      permiteValidar = 1;
   }

   if(permiteValidar == 1)
   {
      sts = SqlCliRecuperarPaisPosibleUSPerson(&cliente);   
      if(sts != SQL_SUCCESS)
      {
         if (sts == SQL_NOT_FOUND)
	     {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no existe.", 0);
	     }
         else
	     {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar posible US Person.", 0);      
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	     }
      }
      
      strcpy(msjCamposEnBlanco, "Debe llenar los siguientes datos: ");
      

      if (strlen(cliente.codigoPais) == 0)
      {
         sinPaisNacionalidad = 1;
         strcat(msjCamposEnBlanco, "Pais de Nacionalidad");
      }      

      if (strlen(cliente.codigoPaisResidencia) == 0)
      {
         sinPaisResidencia = 1;
         if (sinPaisNacionalidad == 0)
            strcat(msjCamposEnBlanco, "Pais de Residencia");
         else
            strcat(msjCamposEnBlanco, ", Pais de Residencia");
      }      

      if (strlen(cliente.codigoPaisNacimiento) == 0)
      {
         sinPaisNacimiento = 1;
         if (sinPaisNacionalidad == 0 && sinPaisResidencia == 0)
            strcat(msjCamposEnBlanco, "Pais de Nacimiento");
         else
            strcat(msjCamposEnBlanco, ", Pais de Nacimiento");
      }
      
      strcat(msjCamposEnBlanco, ". ");
      
      if (sinPaisNacionalidad == 1 || sinPaisResidencia == 1 || sinPaisNacimiento == 1)
	  {      
         strcpy(mensaje, msjCamposEnBlanco);
         esPontecialUsPerson = 0;
      
         Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
         Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
		 Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);      
      }
	  else
	  {
	     sts = SqlCliEsPaisFatca(&cliente, &esPontecialUsPerson);
	     if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si es Pais Fatca.", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
	     
	     if (esPontecialUsPerson == 1){
		 
			sts = SqlCliExisteClienteFatca(clienteFatca, fechaProceso, &existeClienteFatca);
	        if(sts != SQL_SUCCESS)
	        {
		       if (sts == SQL_NOT_FOUND)
		       {
		     	 Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clasificacion de cliente no existe.", 0);
		       }
		       else
		       {
		     	 Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA.", 0);      
		     	 TRX_ABORT(transaccionGlobal);
		     	 tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
		       }
	        }
			
			if (existeClienteFatca == 1)
			{			
			   strcpy(mensaje, "");
			   esPontecialUsPerson = 0;
      
               Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
               Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
			   Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);			
			}
			else
			{			
			   strcpy(msjEsPotencialUsPerson, "Cliente Potencial US Person, Contactar Oficina BackOffice FATCA.");
               strcpy(mensaje, msjEsPotencialUsPerson);
               
               Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
               Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
			   Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);
			}      
         }
		 else
		 {
            strcpy(mensaje, "");
      
            Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
            Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
			Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);
         }
	     

      
      }
   }
   else
   {
      strcpy(mensaje, "");
	  esPontecialUsPerson = 0;

      Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
      Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esPontecialUsPerson, 0);
	  Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);
   
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Cliente FATCA                                        */
/* Autor   : Pablo Valverde               Fecha:  Marzo   2014          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsCliFATCA(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long                 sts=0;
   tCliClienteFATCA     clienteFatca;
   int                  transaccionGlobal;
   short                existeDetalle = 0;
   char                 fechaProceso[8 + 1];
   char                 crearCliente[1 + 1];
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&clienteFatca,0,sizeof(tCliClienteFATCA));
 
   AsignarDeFMLAClienteFATCA(fml, &clienteFatca); 

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Cliente FATCA.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   if (strcmp(clienteFatca.esVigente,CLI_SI)!=0 && strcmp(clienteFatca.esVigente,CLI_NO)!=0) 
      strcpy(crearCliente,CLI_SI);
   else
      strcpy(crearCliente,CLI_NO);

   /********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
   /********* ContinuidadHoraria [FIN] *********/

   if (strlen(clienteFatca.fechaClasificacionFatca)== 0)
   {
      strcpy(clienteFatca.fechaClasificacionFatca, gFechaProceso);
   }

   strcpy(clienteFatca.fechaCreacion, gFechaProceso);
   
   strcpy(clienteFatca.fechaClasificacionDocumental, gFechaProceso);
   strcpy(clienteFatca.fechaExpiracion, gFechaProceso);
   strcpy(fechaProceso, gFechaProceso);

   if (strcmp(crearCliente,CLI_SI)==0) 
       sts = SqlCliExisteClienteFatcaInsertar(clienteFatca, fechaProceso, &existeDetalle);
   else
   sts = SqlCliExisteClienteFatca(clienteFatca, fechaProceso, &existeDetalle);
   if(sts != SQL_SUCCESS)
   {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA.", 0);      
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
	  
   if (strcmp(crearCliente,CLI_NO)==0){
	  strcpy(clienteFatca.esVigente,"N");
	  
	  sts = SqlCliModificarClasificacionFatca(clienteFatca);
      if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar clasificacion cliente FATCA - ERROR: No se pudo modificar clasificacion cliente FATCA.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }   
   
   strcpy(clienteFatca.esVigente,"S");
   
   if (!(strcmp(crearCliente,CLI_SI)==0 && existeDetalle > 0)){
   sts = SqlCliCrearClienteFatca(clienteFatca);
   if (sts != SQL_SUCCESS)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Ingresar clasificacion cliente FATCA - ERROR: No se pudo ingresar clasificacion cliente FATCA.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 
}

/**************************************************************************************/
/* Objetivo: Valida la operacion es un cliente FATCA                                  */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliValOpeFATCA (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;   
   short esOperacionFatca = 0;   
   long sts;
   double operacion = 0.0;
   long sistema = 0;
   
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   
   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_OPERACION, 0, (char *)&operacion, 0);
   Fget32(fml, CLI_CODIGO_SISTEMA, 0, (char *)&sistema, 0);

   sts = SqlCliEsOperacionFatca(operacion, sistema, &esOperacionFatca);   
   if(sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
	  {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no existe.", 0);
	  }
      else
	  {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar posible US Person.", 0);      
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
   }
  
   Fadd32(fml, CLI_FLAG_POTENCIAL_US_PERSON,    (char *)&esOperacionFatca, 0);

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Recupera operaciones cliente FATCA                                       */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecClaFATCA (TPSVCINFO *rqst)
{
   FBFR32                   *fml;
   int                      transaccionGlobal;
   long                     sts;
   Q_HANDLE                 *lista;
   tCliClienteFATCA         clienteFatca;
   tCliClienteFATCA         *cliClienteFatca;
   char                     nuevaCondicion[500 + 1];
   char                     condiciones1[500 + 1];
   char                     condiciones2[500 + 1];
   char                     condiciones3[500 + 1];
   short                    tieneCondicion=0;
   int                      i=0;
   char                     esPersonaJuridica[2];
   
   memset((char *)&clienteFatca, 0, sizeof(tCliClienteFATCA));
   memset(nuevaCondicion, 0, sizeof(nuevaCondicion));
   memset(condiciones1, 0, sizeof(condiciones1));
   memset(condiciones2, 0, sizeof(condiciones2));
   memset(condiciones3, 0, sizeof(condiciones3));
   memset(esPersonaJuridica, 0, sizeof(esPersonaJuridica));

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;   
   
   
   if (Foccur32(fml,CLI_RUTX) > 0){
      Fget32(fml, CLI_RUTX, 0,  (char *)&clienteFatca.rut, 0);
	  sprintf(condiciones1, " rut = %li",clienteFatca.rut);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_OPERACION) > 0){
      Fget32(fml, CLI_OPERACION, 0, (char *)&clienteFatca.operacion, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " operacion = %0.0f",clienteFatca.operacion);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_CODIGO_CLASIFICACION_FATCA) > 0){
      Fget32(fml, CLI_CODIGO_CLASIFICACION_FATCA, 0, (char *)&clienteFatca.codigoClasificacionFatca, 0);
	  
	  if (clienteFatca.codigoClasificacionFatca != 0 )
	  {
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " codigoClasificacionFatca = %d",clienteFatca.codigoClasificacionFatca);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
	  }else{
	     if (tieneCondicion > 0)
            tieneCondicion = 1;
	  }
   }
   
   if (Foccur32(fml,CLI_CODIGO_CLASIFICACION_DOC) > 0){
      Fget32(fml, CLI_CODIGO_CLASIFICACION_DOC, 0, (char *)&clienteFatca.codigoClasificacionDocumental, 0);
	  
	  if (clienteFatca.codigoClasificacionDocumental != 0 )
	  {
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " codigoClasificacionDocumental = %d",clienteFatca.codigoClasificacionDocumental);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
	  }else{
	     if (tieneCondicion > 0)
            tieneCondicion = 1;
	  }
   }
   
   if (Foccur32(fml,CLI_ES_VIGENTE) > 0){
      Fget32(fml, CLI_ES_VIGENTE, 0, clienteFatca.esVigente, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " esVigente = '%s'",clienteFatca.esVigente);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_FECHA_INICIO) > 0){
      Fget32(fml, CLI_FECHA_INICIO, 0, clienteFatca.fechaInicio, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaClasificacionFatca >= TO_DATE('%s','ddmmyyyy')",clienteFatca.fechaInicio);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }

   if (Foccur32(fml,CLI_FECHA_FIN) > 0){
      Fget32(fml, CLI_FECHA_FIN, 0, clienteFatca.fechaFin, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaClasificacionFatca <= TO_DATE('%s','ddmmyyyy')",clienteFatca.fechaFin);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }   
   
   if (Foccur32(fml,CLI_TIPO_PERSONA) > 0){
       Fget32(fml, CLI_TIPO_PERSONA, 0, esPersonaJuridica, 0);

      if (strcmp(esPersonaJuridica,PERSONA_JURIDICA)!=0){

	  if (tieneCondicion > 0)
          strcat(condiciones3, " AND ");     
		 
	  sprintf(nuevaCondicion, " rut < %li",RUT_MINIMO_JURIDICO);
	  strcat(condiciones3, nuevaCondicion);
	  tieneCondicion = 1;

      }else{
	  if (tieneCondicion > 0)
          strcat(condiciones3, " AND ");     
		 
	  sprintf(nuevaCondicion, " rut >= %li",RUT_MINIMO_JURIDICO);
	  strcat(condiciones3, nuevaCondicion);
	  tieneCondicion = 1;

      }
   }else
      condiciones3[0]=0;
  

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarOperacionesFATCA(lista, condiciones1, condiciones2, condiciones3);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen operaciones de Cliente FATCA", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar operaciones de cliente FATCA", 0);
         userlog("%s: Error en funcion SqlCliRecuperarOperacionesFATCA.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)8, (short)8, (short)8, (long)512);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      cliClienteFatca = (tCliClienteFATCA *) QGetItem( lista, i);

      Fadd32(fml, CLI_RUTX,                          (char *)&cliClienteFatca->rut, 0);
      Fadd32(fml, CLI_OPERACION,                     (char *)&cliClienteFatca->operacion, 0);
      Fadd32(fml, CLI_FECHA_CREACION,                cliClienteFatca->fechaCreacion, 0);
      Fadd32(fml, CLI_CODIGO_CLASIFICACION_FATCA,    (char *)&cliClienteFatca->codigoClasificacionFatca, 0);
      Fadd32(fml, CLI_CODIGO_CLASIFICACION_DOC,      (char *)&cliClienteFatca->codigoClasificacionDocumental, 0);
      Fadd32(fml, CLI_FECHA_CLASIFICACION,           cliClienteFatca->fechaClasificacionFatca, 0);
      Fadd32(fml, CLI_FECHA_ESTADO_DOCUMENTAL,       cliClienteFatca->fechaClasificacionDocumental, 0);
      Fadd32(fml, CLI_USUARIO_EJECUTIVO,             (char *)&cliClienteFatca->ejecutivo, 0);
      Fadd32(fml, CLI_SUCURSAL,                      (char *)&cliClienteFatca->sucursal, 0);
      Fadd32(fml, CLI_CODIGO_SISTEMA,                (char *)&cliClienteFatca->sistema, 0);
	  Fadd32(fml, CLI_TIN,                           cliClienteFatca->tin, 0);
	  Fadd32(fml, CLI_COMENTARIO,                    cliClienteFatca->comentario, 0);
	  Fadd32(fml, CLI_ES_VIGENTE,                    cliClienteFatca->esVigente, 0);
	  Fadd32(fml, CLI_FECHAACTUALIZACION,            cliClienteFatca->fechaModificacion, 0);
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Ingresar documentos de cliente FATCA                       */
/* Autor   : Pablo Valverde               Fecha:  Marzo   2014          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliInsDetFATCA(TPSVCINFO *rqst)
{
   FBFR32                  *fml;
   long                    sts;
   tCliDetalleClienteFATCA detalleClienteFatca;
   int                     transaccionGlobal;
   short                   existeDetalle = 0;
   char                    tieneNuevoDocumento[1 + 1];
 
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&detalleClienteFatca,0,sizeof(tCliDetalleClienteFATCA));
   
   Fget32(fml, CLI_TIENE_NUEVO_DOCUMENTO, 0,        tieneNuevoDocumento, 0);
 
   AsignarDeFMLADetalleClienteFATCA(fml, &detalleClienteFatca); 

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Cliente FATCA.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
	  
   /********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
   /********* ContinuidadHoraria [FIN] *********/

   if (strlen(detalleClienteFatca.fechaCreacion)== 0)
   {
      strcpy(detalleClienteFatca.fechaCreacion, gFechaProceso);
   }
 
   sts = SqlCliExisteDetalleClienteFatca(detalleClienteFatca, &existeDetalle);
   if(sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
	  {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Detalle no existe.", 0);
	  }
      else
	  {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe detalle Cliente FATCA.", 0);      
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
   }
   
   if (existeDetalle != 0)
   {   
	  sts = SqlCliModificarDetalleClienteFatca(detalleClienteFatca);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Modificar detalle cliente FATCA - ERROR: No se pudo modificar detalle cliente FATCA.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }

   if (strcmp(tieneNuevoDocumento, "S") == 0)
   {  
      sts = SqlCliIngresarDetalleClienteFatca(detalleClienteFatca);
      if (sts != SQL_SUCCESS)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Ingresar detalle cliente FATCA - ERROR: No se pudo ingresar detalle cliente FATCA.", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 
}

/**************************************************************************************/
/* Objetivo: Recupera estados documental FATCA                                        */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecDocFATCA (TPSVCINFO *rqst)
{
   FBFR32                      *fml;
   int                         transaccionGlobal;
   long                        sts;
   Q_HANDLE                    *lista;
   tCliEstadoDocumentalFATCA   *estadoDocumental;   
   int                         i=0;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarEstadosDocumentalFATCA(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen estados documental FATCA", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar estados documental FATCA", 0);
         userlog("%s: Error en funcion SqlCliRecuperarEstadosDocumentalFATCA.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)2, (short)0, (short)8, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      estadoDocumental = (tCliEstadoDocumentalFATCA *) QGetItem( lista, i);

      Fadd32(fml, CLI_CODIGO_CLASIFICACION_DOC,    (char *)&estadoDocumental->codigo, 0);
      Fadd32(fml, CLI_CLI_DESC_ESTADO_DOCUMENTAL,  estadoDocumental->descripcion, 0);      
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Recupera tipos de documentos FATCA                                        */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecTipoDocF (TPSVCINFO *rqst)
{
   FBFR32                      *fml;
   int                         transaccionGlobal;
   long                        sts;
   Q_HANDLE                    *lista;
   tCliTipoDocumentoFATCA      *tipoDocumento;   
   int                         i=0;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;
   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarTiposDocumentoFATCA(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen estados documental FATCA", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar estados documental FATCA", 0);
         userlog("%s: Error en funcion SqlCliRecuperarTiposDocumentoFATCA.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)2, (short)0, (short)8, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      tipoDocumento = (tCliTipoDocumentoFATCA *) QGetItem( lista, i);

      Fadd32(fml, CLI_CODIGO_DOCUMENTO,          (char *)&tipoDocumento->codigo, 0);
      Fadd32(fml, CLI_DESCRIPCION_DOCUMENTO,     tipoDocumento->descripcion, 0);
	  Fadd32(fml, CLI_TIPO_PERSONA,              tipoDocumento->tipoPersona, 0);
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Recuperar documentos de cliente FATCA                      */
/* Autor   : Pablo Valverde               Fecha:  Marzo   2014          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliRecDetFATCA(TPSVCINFO *rqst)
{
   FBFR32                      *fml;
   int                         transaccionGlobal;
   long                        sts;
   Q_HANDLE                    *lista;
   tCliDetalleClienteFATCA     detalleClienteFatca;
   tCliDetalleClienteFATCA     *detDetalleClienteFatca;
   char                        nuevaCondicion[500 + 1];   
   char                        condiciones1[500 + 1];
   char                        condiciones2[500 + 1];
   char                        condiciones3[500 + 1];
   short                       tieneCondicion=0;
   int                         i=0;
   char                        esPersonaJuridica[2];

   memset((char *)&detalleClienteFatca, 0, sizeof(tCliDetalleClienteFATCA));
   memset(nuevaCondicion, 0, sizeof(nuevaCondicion));
   memset(condiciones1, 0, sizeof(condiciones1));
   memset(condiciones2, 0, sizeof(condiciones2));
   memset(condiciones3, 0, sizeof(condiciones3));

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;   
   
   if (Foccur32(fml,CLI_RUTX) > 0){
      Fget32(fml, CLI_RUTX, 0,  (char *)&detalleClienteFatca.rut, 0);
	  sprintf(condiciones1, " rut = %li",detalleClienteFatca.rut);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_CORRELATIVO) > 0){
      Fget32(fml, CLI_CORRELATIVO, 0,  (char *)&detalleClienteFatca.correlativo, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");
		 
	  sprintf(nuevaCondicion, " correlativo = %li",detalleClienteFatca.correlativo);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_OPERACION) > 0){
      Fget32(fml, CLI_OPERACION, 0,  (char *)&detalleClienteFatca.operacion, 0);	  
	  
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " operacion = %0.0f",detalleClienteFatca.operacion);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_CODIGO_DOCUMENTO) > 0){
      Fget32(fml, CLI_CODIGO_DOCUMENTO, 0, (char *)&detalleClienteFatca.tipoDocumento, 0);
	  
	  if (detalleClienteFatca.tipoDocumento != 0 )
	  {
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " tipoDocumento = %d",detalleClienteFatca.tipoDocumento);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
	  }else{
	     if (tieneCondicion > 0)
            tieneCondicion = 1;
	  }
   }
   
   if (Foccur32(fml,CLI_CODIGO_CLASIFICACION_DOC) > 0){
      Fget32(fml, CLI_CODIGO_CLASIFICACION_DOC, 0, (char *)&detalleClienteFatca.codigoEstadoDocumento, 0);
	  
	  if (detalleClienteFatca.codigoEstadoDocumento != 0 )
	  {
	  if (tieneCondicion > 0)
         strcat(condiciones1, " AND ");     
		 
	  sprintf(nuevaCondicion, " codigoEstadoDocumento = %d",detalleClienteFatca.codigoEstadoDocumento);
	  strcat(condiciones1, nuevaCondicion);
	  tieneCondicion = 1;
	  }else{
	     if (tieneCondicion > 0)
            tieneCondicion = 1;
	  }
   }
   
   if (Foccur32(fml,CLI_ES_VIGENTE) > 0){
      Fget32(fml, CLI_ES_VIGENTE, 0, detalleClienteFatca.esVigente, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " esVigente = '%s'",detalleClienteFatca.esVigente);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_FECHA_INICIO) > 0){
      Fget32(fml, CLI_FECHA_INICIO, 0, detalleClienteFatca.fechaSolicitudInicio, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaCreacion >= TO_DATE('%s','ddmmyyyy')",detalleClienteFatca.fechaSolicitudInicio);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }

   if (Foccur32(fml,CLI_FECHA_FIN) > 0){
      Fget32(fml, CLI_FECHA_FIN, 0, detalleClienteFatca.fechaSolicitudFin, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaCreacion <= TO_DATE('%s','ddmmyyyy')",detalleClienteFatca.fechaSolicitudFin);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_FECHA_INICIO_2) > 0){
      Fget32(fml, CLI_FECHA_INICIO_2, 0, detalleClienteFatca.fechaEntregaInicio, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaEntrega >= TO_DATE('%s','ddmmyyyy')",detalleClienteFatca.fechaEntregaInicio);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }

   if (Foccur32(fml,CLI_FECHA_FIN_2) > 0){
      Fget32(fml, CLI_FECHA_FIN_2, 0, detalleClienteFatca.fechaEntregaFin, 0);
	  
	  if (tieneCondicion > 0)
         strcat(condiciones2, " AND ");     
		 
	  sprintf(nuevaCondicion, " fechaEntrega <= TO_DATE('%s','ddmmyyyy')",detalleClienteFatca.fechaEntregaFin);
	  strcat(condiciones2, nuevaCondicion);
	  tieneCondicion = 1;
   }
   
   if (Foccur32(fml,CLI_TIPO_PERSONA) > 0){
      Fget32(fml, CLI_TIPO_PERSONA, 0, esPersonaJuridica, 0);
      if (strcmp(esPersonaJuridica,PERSONA_JURIDICA)!=0){

          if (tieneCondicion > 0)
          strcat(condiciones3, " AND ");

          sprintf(nuevaCondicion, " rut < %li",RUT_MINIMO_JURIDICO);
          strcat(condiciones3, nuevaCondicion);
          tieneCondicion = 1;

      }else{
          if (tieneCondicion > 0)
          strcat(condiciones3, " AND ");

          sprintf(nuevaCondicion, " rut >= %li",RUT_MINIMO_JURIDICO);
          strcat(condiciones3, nuevaCondicion);
          tieneCondicion = 1;

      }
   }else
       condiciones3[0]=0;

   
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarDocumentosClienteFATCA(lista, condiciones1, condiciones2, condiciones3);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen estados documentos de cliente FATCA", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar documentos de cliente FATCA", 0);
         userlog("%s: Error en funcion SqlCliRecuperarDocumentosClienteFATCA.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)8, (short)8, (short)8, (long)512);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      detDetalleClienteFatca = (tCliDetalleClienteFATCA *) QGetItem( lista, i);

	  Fadd32(fml, CLI_CORRELATIVO,                  (char *)&detDetalleClienteFatca->correlativo, 0);
	  Fadd32(fml, CLI_OPERACION,                    (char *)&detDetalleClienteFatca->operacion, 0);
      Fadd32(fml, CLI_CODIGO_DOCUMENTO,             (char *)&detDetalleClienteFatca->tipoDocumento, 0);
      Fadd32(fml, CLI_RUTX,                         (char *)&detDetalleClienteFatca->rut, 0);
      Fadd32(fml, CLI_FECHA_CREACION,               detDetalleClienteFatca->fechaCreacion, 0);
	  Fadd32(fml, CLI_FECHA_ENTREGA,                detDetalleClienteFatca->fechaEntrega, 0);
      Fadd32(fml, CLI_FECHA_TOPE,                   detDetalleClienteFatca->fechaTope, 0);
      Fadd32(fml, CLI_CODIGO_CLASIFICACION_DOC,     (char *)&detDetalleClienteFatca->codigoEstadoDocumento, 0);
      Fadd32(fml, CLI_USUARIO_EJECUTIVO,            (char *)&detDetalleClienteFatca->ejecutivo, 0);
      Fadd32(fml, CLI_SUCURSAL,                     (char *)&detDetalleClienteFatca->sucursal, 0);
	  Fadd32(fml, CLI_ES_VIGENTE,                   detDetalleClienteFatca->esVigente, 0);
	  Fadd32(fml, CLI_FECHAACTUALIZACION,           detDetalleClienteFatca->fechaModificacion, 0);
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
  
}

/************************************************************************/
/* Objetivo: Modificar documentos de cliente FATCA                      */
/* Autor   : Pablo Valverde               Fecha:  Marzo   2014          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModDetFATCA(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliDetalleClienteFATCA detalleClienteFatca;
   int transaccionGlobal;
 
   /******   Buffer de entrada   *****/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&detalleClienteFatca,0,sizeof(tCliDetalleClienteFATCA));
 
   AsignarDeFMLADetalleClienteFATCA(fml, &detalleClienteFatca); 

   /******   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Cliente FATCA.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarDetalleClienteFatca(detalleClienteFatca);
   if (sts != SQL_SUCCESS)   
   {
      if (sts == SQL_NOT_FOUND)
      {	  
		  Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe detalle de cliente FATCA.", 0);
          TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);		  
	  }
      else
	  {
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar detalle de cliente FATCA.", 0);
		  TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
   }
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 
}

/************************************************************************/
/* Objetivo: Modificar clasificacion de cliente FATCA                   */
/* Autor   : Pablo Valverde               Fecha:  Marzo   2014          */
/* Modifica:                              Fecha:                        */
/************************************************************************/
void CliModClaFATCA(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   long sts;
   tCliClienteFATCA clienteFatca;
   int transaccionGlobal;
 
   /******   Buffer de entrada   *****/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&clienteFatca,0,sizeof(tCliClienteFATCA));
 
   AsignarDeFMLAClienteFATCA(fml, &clienteFatca); 

   /******   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
 
   if (transaccionGlobal == 0)
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Cliente FATCA.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
 
   sts = SqlCliModificarClasificacionFatca(clienteFatca);
   if (sts != SQL_SUCCESS)   
   {
      if (sts == SQL_NOT_FOUND)
      {	  
		  Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe detalle de cliente FATCA.", 0);
          TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);		  
	  }
      else
	  {
          Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar clasificacion de cliente FATCA.", 0);
		  TRX_ABORT(transaccionGlobal);
          tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
   }
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 
}

/**************************************************************************************/
/* Objetivo: Recupera tipos de clasificacion FATCA                                    */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecTipoClaF (TPSVCINFO *rqst)
{
   FBFR32                      *fml;
   int                         transaccionGlobal;
   long                        sts;
   Q_HANDLE                    *lista;
   tCliClasificacionFATCA      *tipoClasificacionFATCA;   
   int                         i=0;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;
 
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarTiposClasificacionFATCA(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen estados documental FATCA", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar estados documental FATCA", 0);
         userlog("%s: Error en funcion SqlCliRecuperarTiposClasificacionFATCA.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)lista->count, (short)2, (short)0, (short)8, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      tipoClasificacionFATCA = (tCliClasificacionFATCA *) QGetItem( lista, i);

      Fadd32(fml, CLI_CODIGO_DOCUMENTO,          (char *)&tipoClasificacionFATCA->codigo, 0);
      Fadd32(fml, CLI_DESCRIPCION_DOCUMENTO,     tipoClasificacionFATCA->descripcion, 0);
	  Fadd32(fml, CLI_TIPO_PERSONA,              tipoClasificacionFATCA->tipoPersona, 0);
	  Fadd32(fml, CLI_ES_TERMINAL,               tipoClasificacionFATCA->esTerminal, 0);
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Recupera operaciones de Potencial US Person                              */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecOpeFATCA (TPSVCINFO *rqst)
{
   FBFR32                      *fml;
   int                         transaccionGlobal;
   long                        sts;
   Q_HANDLE                    *lista;
   tCliClienteFATCA            *clienteFatca;
   long                        rut=0;
   int                         i=0;

   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0,  (char *)&rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarOperacionesPotencialUSPerson(rut, lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen operaciones de cliente potencial US Person", 0);
      else{
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar de cliente potencial US Person", 0);
         userlog("%s: Error en funcion SqlCliRecuperarOperacionesPotencialUSPerson.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
	  }
   }

   fml = NewFml32((long)lista->count, (short)2, (short)0, (short)8, (long)80);
   if (fml == NULL)
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, 1024);
      Fadd32(fml, HRM_MENSAJE_SERVICIO, CLI_TEXTO_GENERICO_DE_ERROR, 0);
      userlog("%s: Error en funcion NewFml32.", rqst->name);
      QDelete(lista);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < lista->count; i++)
   {
      clienteFatca = (tCliClienteFATCA *) QGetItem( lista, i);

      Fadd32(fml, CLI_RUTX,                          (char *)&clienteFatca->rut, 0);
      Fadd32(fml, CLI_OPERACION,                     (char *)&clienteFatca->operacion, 0);
	  
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
/* Implantación de Normativa FATCA PJVA FIN */

/************************************************************************/
/* Objetivo: Recuperar Informacion Cliente para TCr                     */
/* Autor   : Felix Salgado I.                 Fecha: 18-07-2014         */
/************************************************************************/
void CliRecInfoCliTC(TPSVCINFO *rqst)
{
   FBFR32     *fml;
   long       sts;
   int        transaccionGlobal;
   tCliCLIENTETCR clienteTcr;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&clienteTcr.rut, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   TRX_BEGIN(transaccionGlobal);

   sts = SqlTcrRecuperarInformacionClienteTCr(&clienteTcr);

   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existe Información de Cliente", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar Información de Cliente", 0);
     TRX_ABORT(transaccionGlobal);
     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml=(FBFR32 *)tpalloc("FML32",NULL,ESTIMAR_MEMORIA(11,sizeof(tCliCLIENTETCR)));

   Fadd32( fml, CLI_RUT, (char *)&clienteTcr.rut,0);
   Fadd32( fml, CLI_NOMBRE, clienteTcr.nombres,0);
   Fadd32( fml, CLI_APELLIDOPATERNO, clienteTcr.apellidoPaterno,0);
   Fadd32( fml, CLI_APELLIDOMATERNO, clienteTcr.apellidoMaterno,0);
   Fadd32( fml, CLI_CALLE_NUMERO, clienteTcr.calleNumero,0);
   Fadd32( fml, CLI_VILLA_POBLACION, clienteTcr.villaPoblacion,0);
   Fadd32( fml, CLI_DESCRIPCION_COMUNA, clienteTcr.nombreComuna,0);
   Fadd32( fml, CLI_DESCRIPCION_CIUDAD, clienteTcr.nombreCiudad,0);
   Fadd32( fml, CLI_CLASIFICACION, clienteTcr.categoriaCliente,0);
   Fadd32( fml, CLI_SEXOX, (char *)&clienteTcr.sexo,0);
   Fadd32( fml, CLI_ESTADO_CIVIL, (char *)&clienteTcr.estadoCivil,0);
   Fadd32( fml, CLI_DVX, clienteTcr.dv,0);
   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Valida la rut de cliente FATCA                                  */
/* Autor   : Pablo Valverde               Fecha:  marzo   2014                        */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliValRutFATCA (TPSVCINFO *rqst)
{
   FBFR32           *fml;
   int              transaccionGlobal;
   tCliClienteFATCA clienteFatca;
   char             fechaProceso[8 + 1];   
   char             mensaje[250 + 1];
   char             msjEsPotencialUsPerson[250+1];
   short            existeClienteFatca = 0;   
   long             sts;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&clienteFatca,0,sizeof(tCliClienteFATCA));
   
   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_RUTX, 0, (char *)&clienteFatca.rut, 0);
   
   /********* ContinuidadHoraria [INI] *********/
   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);   
   }
   /********* ContinuidadHoraria [FIN] *********/
   
   strcpy(fechaProceso, gFechaProceso);
   
   sts = SqlCliExisteRutClienteFatca(clienteFatca, fechaProceso, &existeClienteFatca);
   if(sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Clasificacion de cliente no existe.", 0);
      }
      else
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA.", 0);      
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }
   }
   
   if (existeClienteFatca == 1)
   {			
      strcpy(msjEsPotencialUsPerson, "Cliente posee clasificacion FATCA. Cambiar la fecha de expiracion de la actual clasficación para insertar una nueva.");
	  strcpy(mensaje, msjEsPotencialUsPerson);
      
      Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
      Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);			
   }
   else
   {
      strcpy(mensaje, "");
   
      Fadd32(fml, CLI_MENSAJE_FATCA,               mensaje, 0);
      Fadd32(fml, CLI_ES_CLIENTE_FACTA,            (char *)&existeClienteFatca, 0);
   }    

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Valida el correo electronico                                             */
/* Autor   : francisco Valdivia           Fecha:  octubre   2013                      */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliValidarMail (TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   char email[80+1];
   long sts;
   int i;
   long existe;
   char respuesta[100+1];
  

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /*****   Cuerpo del servicio   *****/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_EMAIL, 0, email, 0);


   sts = SqlCliValidarMail(&existe, email);
   if (sts != SQL_SUCCESS && sts != SQL_NOT_FOUND)
   {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar correo electronico", 0);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
   }

   if(existe == 2){
        strcpy(respuesta, "El Correo Electrónico no cumple con el formato exigido por el Banco");
   }else{
	strcpy(respuesta, "OK");
   }

   Fadd32(fml, CLI_RESPUESTA_FIJA,      respuesta,     0);


   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/*********************************************************************************/
/* Objetivo: Actualiza Forma envio Mail Ficha Cliente                            */
/* Autor   : Felix Salgado                                     Fecha: 13-11-2013 */
/* Modifica:                                                   Fecha:            */
/*********************************************************************************/
void CliModFormaEnv(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliFormaEnvioMail formaEnvioMail;
   char actualizarMail = 'N';

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   memset((char *)&formaEnvioMail,0,sizeof(tCliFormaEnvioMail));


   if (Foccur32(fml,CLI_EMAIL) > 0){
       actualizarMail = 'S';
       Fget32(fml, CLI_EMAIL,0, formaEnvioMail.mail,0);
       userlog("CliModFormaEnv CLI_EMAIL -> [%s]\n",formaEnvioMail.mail);
   }

   Fget32(fml, CLI_RUT,               0, (char *)&formaEnvioMail.rut,   0);
   Fget32(fml, CLI_FORMAENVIOCARTOLA, 0, formaEnvioMail.formaEnvioCartola,        0);

	
userlog("FVE CLI_RUT -->[%li]\n", formaEnvioMail.rut);
userlog("FVE CLI_FORMAENVIOCARTOLA -->[%s]\n", formaEnvioMail.formaEnvioCartola);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);
	

   if(actualizarMail == 'S'){
	 sts = SqlCliModificaFormaEnvioMail(formaEnvioMail);
   }else{
	 sts = SqlCliModificaFormaEnvio(formaEnvioMail);
   }
  
   
   if (sts != SQL_SUCCESS)
   {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al modificar forma envio mail", 0);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/**************************************************************************************/
/* Objetivo: Recuperar motivo de modificacion en verificacion de domicilio            */
/* Autor   : Cristobal Salazar            Fecha: Febrero 2015                         */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliRecMotModVD(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliCODIGOGLOSA *motivo;

   Q_HANDLE *lista;
   int totalRegistros=0, i=0;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      sts = SQL_MEMORY;
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "12010619 Error de sistema.", 0);
      tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMotivoModificacion(lista);
   if (sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "12010620 No existen motivo.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "12010621 Error al recuperar las motivo.", 0);

      QDelete(lista);
      tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros=lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, 100000);

   for (i=0; i < lista->count ; i++)
   {
      motivo = (tCliCODIGOGLOSA *)QGetItem(lista, i);
   
      Fadd32(fml, CLI_CODIGO, (char *)&motivo->codigo, 0);
      Fadd32(fml, CLI_DESCRIPCION, motivo->descripcion, 0);
   }

   QDelete(lista);
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



/************************************************************************/
/* Objetivo: Recupera Mensajes PEP                                      */
/* Autor   : Felix Salgado                            Fecha: 02-10-2015 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecMsjPEP(TPSVCINFO *rqst)
{
   FBFR32                   *fml;
   int                      transaccionGlobal;
   long                     sts;
   Q_HANDLE                 *lista;
   tCliMENSAJESPEP   *mensajePep;
   int totalRegistros=0, i=0;


   /***   Buffer de entrada   ***/
   fml = (FBFR32 *)rqst->data;


   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCliRecuperarMensajesClientePEP(lista);
   if (sts != SQL_SUCCESS)
   {
      if ( sts == SQL_NOT_FOUND )
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registros de mensajes cliente PEP", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen registros de mensajes cliente PEP", 0);
         userlog("%s: Error en funcion SqlCliRecuperarMensajesClientePEP.", rqst->name);
         QDelete(lista);
         TRX_ABORT(transaccionGlobal);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   totalRegistros = lista->count;
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(4 * totalRegistros, sizeof(tCliMENSAJESPEP) * totalRegistros));

   for (i=0; i < lista->count; i++)
   {
      mensajePep = (tCliMENSAJESPEP *) QGetItem( lista, i);

      Fadd32(fml, CLI_TIPO_MENSAJE_PEP,                        (char*)&mensajePep->tipo, 0);
      Fadd32(fml, CLI_DESCRIPCION_TIPO_MENSAJE_PEP,             mensajePep->descripcionTipo, 0);
      Fadd32(fml, CLI_MENSAJE_CLIENTE_PEP,                      mensajePep->mensaje, 0);
      Fadd32(fml, CLI_ESTADO_MENSAJE_PEP,                      (char*)&mensajePep->estadoMensaje, 0);      
 
   }

   QDelete( lista );
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}


/************************************************************************/
/* Objetivo: Modifica - Elimina Mensajes PEP                            */
/* Autor   : Felix Salgado                            Fecha: 02-10-2015 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliMantMsjPep(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   char flagMensaje[1 + 1];
   tCliMENSAJESPEP mensajePep;

   memset(&mensajePep,0,sizeof(tCliMENSAJESPEP));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_TIPO_MENSAJE_PEP, 0, (char *)&mensajePep.tipo, 0);
   Fget32(fml, CLI_MENSAJE_CLIENTE_PEP, 0, mensajePep.mensaje, 0);
   Fget32(fml, CLI_ESTADO_MENSAJE_PEP, 0, (char *)&mensajePep.estadoMensaje, 0);
   Fget32(fml, CLI_DESCRIPCION_TIPO_MENSAJE_PEP, 0, mensajePep.descripcionTipo, 0);
   Fget32(fml, CLI_FLAG_MENSAJE_PEP, 0, flagMensaje, 0);

   
    if(strcmp(flagMensaje,ELIMINAR_MENSAJE_PEP)==0){
	
	 sts = SqlCliEliminarMensajePep(mensajePep);
	 if (sts != SQL_SUCCESS)
	 {
	     Fadd32(fml, HRM_MENSAJE_SERVICIO, "SqlCliEliminarMensajePep: Error al eliminar mensaje PEP.", 0);
	     TRX_ABORT(transaccionGlobal);	
	     tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
	 }
		
	}else if(strcmp(flagMensaje,MODIFICAR_MENSAJE_PEP)==0){
	
	     sts = SqlCliModificarMensajePep(mensajePep);
	     if (sts != SQL_SUCCESS)
	     {
		  Fadd32(fml, HRM_MENSAJE_SERVICIO, "SqlCliModificarMensajePep: Error al modificar mensaje PEP.", 0);
		  TRX_ABORT(transaccionGlobal);		
		  tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
	     }	
	}else if(strcmp(flagMensaje,INSERTAR_MENSAJE_PEP)==0){
	
	    sts = SqlCliInsertarMensajePep(mensajePep);
	    if (sts != SQL_SUCCESS)
	    {
                  if (sts == SQL_DUPLICATE_KEY)
		      Fadd32(fml, HRM_MENSAJE_SERVICIO, "El código de mensaje PEP ya existe, debe ingresar uno distinto.", 0);
                  else
		      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error grave al insertar mensaje PEP.", 0);

		  TRX_ABORT(transaccionGlobal);		
		  tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
	    }	
        }

   
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Crear Cliente FATCA / Impresion Documentos FATCA           */
/* Autor   : Cristobal Salazar            Fecha: 08 de Febrero 2016     */
/* Modifica:                                                            */
/************************************************************************/
void CliStockFATCA(TPSVCINFO *rqst)
{
   FBFR32               *fml;
   long                 sts=0;
   tCliClienteFATCA     clienteFatca;
   tCliClienteFATCA     clienteFatcaOpe;
   int                  transaccionGlobal;
   short                existeDetalle = 0;
   short                existeDetallePais = 0;
   char                 fechaProceso[8 + 1];
   Q_HANDLE             *lista;

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;
   memset((char *)&clienteFatca,0,sizeof(tCliClienteFATCA));

   Fget32(fml, CLI_RUTX, 0, (char *)&clienteFatca.rut, 0);
   Fget32(fml, CLI_USUARIO, 0, (char *)&clienteFatca.ejecutivo, 0);
   Fget32(fml, CLI_SUCURSAL, 0, (char *)&clienteFatca.sucursal, 0);


   userlog("RUT CLIENTE-------->:[%li]\n", clienteFatca.rut);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();

   if (transaccionGlobal == 0)
   {
      if (tpbegin(TIME_OUT, 0L) == -1)
      {
         Fadd32(fml,HRM_MENSAJE_SERVICIO, "Error al iniciar Transaccion Crear Cliente FATCA.", 0);
         tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
      }
   }

   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      userlog("%s: Error en QNew_", rqst->name);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlRecuperarParametrosSGT(gFechaProceso, gFechaCalendario);
   if (sts != SQL_SUCCESS)
   {
      userlog("Falla recuperacion de parametros de proceso (SGT)");
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Falla recuperacion de parametros de proceso (SGT)", 0);
      TRX_ABORT(transaccionGlobal);
      QDelete(lista);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }

   if (strlen(clienteFatca.fechaClasificacionFatca)== 0)
   {
      strcpy(clienteFatca.fechaClasificacionFatca, gFechaProceso);
   }

   strcpy(clienteFatca.fechaCreacion, gFechaProceso);
   strcpy(clienteFatca.fechaClasificacionDocumental, gFechaProceso);
   strcpy(clienteFatca.fechaExpiracion, gFechaProceso);
   strcpy(fechaProceso, gFechaProceso);

   sts = SqlCliExisteRutClienteFatca(clienteFatca, fechaProceso, &existeDetalle);
   if(sts != SQL_SUCCESS)
   {
      if (sts != SQL_NOT_FOUND)
      {
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA.", 0);
         TRX_ABORT(transaccionGlobal);
         QDelete(lista);
         tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
      }

      existeDetalle = 0;
   }

   if (existeDetalle == 0){

      sts = SqlCliExisteNacionalidadClienteFatca(clienteFatca, &existeDetallePais);
      if(sts != SQL_SUCCESS)
      {
         if (sts != SQL_NOT_FOUND)
         {
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe pais FATCA.", 0);
            TRX_ABORT(transaccionGlobal);
            QDelete(lista);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
         existeDetallePais = 0;
      }

      if (existeDetallePais != 0){

         sts = SqlCliRecuperarOperacionesProductoFatca(clienteFatca.rut, lista);
         if(sts != SQL_SUCCESS)
         {
            if (sts != SQL_NOT_FOUND)
            {
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe clasificacion Cliente FATCA[SqlCliRecuperarOperacionesProductoFatca].", 0);
               TRX_ABORT(transaccionGlobal);
               QDelete(lista);
               tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
            }
         }

         if (lista->count == 0){

               sts = SRV_CRITICAL_ERROR;
               Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no posee operaciones fatca , no se puede crear registro de detalle operaciones", 0);
               TRX_ABORT(transaccionGlobal);
               QDelete(lista);
               tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);

         }

         clienteFatcaOpe = *(tCliClienteFATCA *) QGetItem( lista, 0);

         strcpy(clienteFatca.esVigente,"S");
         clienteFatca.codigoClasificacionFatca = CODIGO_CLASIF_FATCA_POTENCIAL_US_PERSON;
         clienteFatca.codigoClasificacionDocumental = DOCUMENTO_SIN_CLASIFICAR;
         clienteFatca.operacion = clienteFatcaOpe.operacion;;
         clienteFatca.sistema = 0;
         clienteFatca.tin[0] = 0;
         clienteFatca.comentario[0] = 0;

         sts = SqlCliCrearClienteFatca(clienteFatca);
         if (sts != SQL_SUCCESS)
         {
            QDelete(lista);
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Ingresar clasificacion cliente FATCA - ERROR: No se pudo ingresar clasificacion cliente FATCA.", 0);
            TRX_ABORT(transaccionGlobal);
            tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
         }
         existeDetalle = 1;
         existeDetallePais = 1;
      }

    }else
        existeDetallePais = 1;

    if (existeDetalle == 0 || existeDetallePais == 0){

       sts = SRV_CRITICAL_ERROR;
       Fadd32(fml, HRM_MENSAJE_SERVICIO, "Cliente no posee las condiciones para registrarlo como Cliente Fatca.", 0);
       TRX_ABORT(transaccionGlobal);
       QDelete(lista);
       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);

    }

    QDelete(lista);
    TRX_COMMIT(transaccionGlobal);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}

/* Portal Empresarial Fase II CSC */
/************************************************************************/
/* Objetivo: Recupera listado de empresas de servicio                   */
/* Autor   : Cristobal Sepulveda                      Fecha: 26-11-2013 */
/* Modifica:                                          Fecha:            */
/************************************************************************/
void CliRecEmpServ(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   Q_HANDLE *listaEmpServ;
   tCliEmpresaServicio *empresas;
   tCliEmpresaServicio empresa;
   int i;
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   printf("Servicio [%s]\n",rqst->name);
   Fprint32(fml);
   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   Fget32(fml, CLI_ID_SERVICIO,  0, (char *)&empresa.id_servicio, 0);
   Fget32(fml, CLI_NOM_SERVICIO, 0, empresa.nom_servicio,         0);

   if ((listaEmpServ = QNew_(10,10)) == NULL)
   {
      sts = SQL_MEMORY;
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema.", 0);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name),(char *)fml, 0L, 0L);
   }

   sts = SqlCarRecEmpServ(&empresa, listaEmpServ);
   if(sts != SQL_SUCCESS)
   {
      if (sts == SQL_NOT_FOUND)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "No existen empresas de servicio con los datos indicados", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar Empresas.", 0);
      QDelete(listaEmpServ);
      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   fml = NewFml32((long)listaEmpServ->count, (short)1, (short)0, (short)2, (long)200);

   if (fml == NULL)  /* Falla al estimar memoria */
   {
      fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(3, 500));
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error interno en el Servidor", 0);
      QDelete(listaEmpServ);

      TRX_ABORT(transaccionGlobal);
      tpreturn(TPFAIL, ErrorServicio(SRV_MEMORY, rqst->name), (char *)fml, 0L, 0L);
   }

   for (i=0; i < listaEmpServ->count ; i++)
   {
      empresas = (tCliEmpresaServicio *)QGetItem(listaEmpServ,i);

      Fadd32(fml, CLI_ID_BILLER,    (char *)&empresas->id_biller, 0);
      Fadd32(fml, CLI_NOM_BILLER,   empresas->nom_biller,         0);
      Fadd32(fml, CLI_NOM_FANTASIA, empresas->nom_fantasia,       0);
   }
   QDelete(listaEmpServ);

   TRX_COMMIT(transaccionGlobal);

   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/* Portal Empresarial Fase II CSC */
/*********************************************************/
/*                    VHMG      2017                     */
/*********************************************************/


void CliInsCliCRS (TPSVCINFO *rqst)
{
    FBFR32   *fml;
    int    transaccionCliente;
    tcliClienteTributario   cliClienteTributario;

    long   stsCli;
    long   i;
    long   contadorOcurrecias = 0;
    char   strVigencia;
    long   contVigencia;
    short  contador=0;
    char   mensaje[100];

   /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    transaccionCliente = tpgetlev();
    TRX_BEGIN(transaccionCliente);
 
    mensaje[0]=0;
 
    contadorOcurrecias = Foccur32(fml, CLI_RUTX);

    contVigencia=0;

    for (i=0; i<= contadorOcurrecias-1; i++)
     {
         Fget32(fml, CLI_RUTX,            i, (char *)&cliClienteTributario.rut,   0);
         Fget32(fml, CLI_CLIENTE_TIN,     i, cliClienteTributario.tin,            0);
         Fget32(fml, CLI_DOMICILIO_CRS,   i, cliClienteTributario.domicilioCrs,   0); 
         Fget32(fml, CLI_COD_PAIS,        i, cliClienteTributario.codPais,        0);  

         trim(cliClienteTributario.tin); trim(cliClienteTributario.codPais); trim(cliClienteTributario.domicilioCrs);
         contador=0;
         stsCli = SqlCliExisteCRSPorTin(cliClienteTributario.tin,cliClienteTributario.codPais,&contador);
         if (stsCli != SQL_SUCCESS){
          
            printf("error al recuperar clientecrs por tin y pais\n");
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al validar si existe tin/pais", 0);
            TRX_ABORT(transaccionCliente);
            tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
         }

         if (contador > 0){
            sprintf(mensaje,"El tin y pais que esta ingresando ya se encuentra registrado. Revisar los datos de ingreso (tin:%s-pais%s)",
                   cliClienteTributario.tin,cliClienteTributario.codPais);
            printf(mensaje);
            Fadd32(fml, HRM_MENSAJE_SERVICIO, mensaje, 0);
            TRX_ABORT(transaccionCliente);
            tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
         }

         stsCli=SqlCliAgregarClienteTributario(cliClienteTributario,CLI_SI);  

         if (stsCli != SQL_SUCCESS){
          
            printf("error en ingreso de cliente tributario\n");
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar direcciones tributarias", 0);
            TRX_ABORT(transaccionCliente);
            tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
          }
        else
          printf("ingreso de cliente exitoso tributario \n");
	    
    } 

   TRX_COMMIT(transaccionCliente);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}



void CliActCliCRS (TPSVCINFO *rqst)
{
    tcliClienteTributario   cliClienteTributario; 
    FBFR32   *fml;
    int    transaccionCliente;
    char   incluyeOfertaMantencion[2];
    char   incluyeLCC[2];
    char   incluyeLCD[2];
    long   stsCli;
    long   i=0;
    long   contadorOcurrecias = 0;
    char   strVigencia;
    long   contVigencia=0;
	
    transaccionCliente = tpgetlev();
    TRX_BEGIN(transaccionCliente);

   /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    contadorOcurrecias = Foccur32(fml,CLI_RUTX);

Fprint32(fml);

    for (i=0; i<= contadorOcurrecias-1; i++)
     {  

        Fget32(fml, CLI_RUTX,            i, (char *)&cliClienteTributario.rut,   0);
        Fget32(fml, CLI_CLIENTE_TIN,     i, cliClienteTributario.tin,            0);
        Fget32(fml, CLI_ES_VIGENTE,      i, cliClienteTributario.esVigente,      0);
        Fget32(fml, CLI_DOMICILIO_CRS,   i, cliClienteTributario.domicilioCrs,   0); 
        Fget32(fml, CLI_COD_PAIS,        i, cliClienteTributario.codPais,        0);  

        trim(cliClienteTributario.tin);
        trim(cliClienteTributario.domicilioCrs);
        trim(cliClienteTributario.codPais);
        stsCli=SqlCliEliminarClienteTributario(cliClienteTributario);
 
        if (stsCli != SQL_SUCCESS)
         {
            printf("error en Actualizar/Elim  Campo CRS en cliente\n");
            Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al Eliminar cliente ", 0);
            TRX_ABORT(transaccionCliente);
            tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
         }
              

        if (strcmp(cliClienteTributario.esVigente,CLI_SI)==0){
          stsCli=SqlCliAgregarClienteTributario(cliClienteTributario,CLI_SI);
          if (stsCli != SQL_SUCCESS)
           {
              printf("error en Actualizar cliente CRS\n");
              Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al actualizar cliente ( CRS)", 0);
              TRX_ABORT(transaccionCliente);
              tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
           }
        }

    
        if (cliClienteTributario.esVigente[0] == 'S')
          contVigencia++;

	    
    }

    TRX_COMMIT(transaccionCliente);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);

}


void CliRecCliCRS(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionCliente;
   long stsT;
   Q_HANDLE *lista;
   long      totalRegistros=0;
   long      i;
   long rut=0;
   tcliClienteTributario *cliTrib;
   
   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fprint32(fml);
   /******   Cuerpo del servicio   ******/
   transaccionCliente = tpgetlev();
   TRX_BEGIN(transaccionCliente);
  
   if ((lista = QNew_(10,10)) == NULL)
   {
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error de sistema", 0);
      TRX_ABORT(transaccionCliente);
      tpreturn(TPFAIL, ErrorServicio(SRV_CRITICAL_ERROR, rqst->name), (char *)fml, 0L, 0L);
   }
 
   Fget32(fml, CLI_RUTX,     0, (char *)&rut,   0);   
      
   stsT=SqlCliRecuperaRutClienteCRS(lista, rut);
   totalRegistros = lista->count;
   userlog("tot Rec : %d", totalRegistros);
   fml = (FBFR32 *) tpalloc("FML32", NULL, ESTIMAR_MEMORIA(2 * totalRegistros, sizeof(tcliClienteTributario)* totalRegistros));
   for (i = 0; i < totalRegistros ; i++)
   {
        
        cliTrib = (tcliClienteTributario *)QGetItem(lista, i);
        Fadd32(fml, CLI_RUTX,             (char *)&cliTrib->rut, 0);
        Fadd32(fml, CLI_CLIENTE_TIN,      cliTrib->tin, 0);
	Fadd32(fml, CLI_FECHA_CREACION,   cliTrib->fechaCreacion, 0); 
	Fadd32(fml, CLI_DOMICILIO_CRS,    cliTrib->domicilioCrs, 0);
	Fadd32(fml, CLI_COD_PAIS,         cliTrib->codPais, 0);  
  }	 
  TRX_COMMIT(transaccionCliente);
  tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 } 


void CliEliCliCRS(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionCliente;
   long stsT;
   Q_HANDLE *lista;
   long      totalRegistros=0;
   long      i;
   tcliClienteTributario cliClienteTributario;;
   long   contadorOcurrecias = 0;
   long   stsCli;

   fml = (FBFR32 *)rqst->data;

   /******   Cuerpo del servicio   ******/
   transaccionCliente = tpgetlev();
   TRX_BEGIN(transaccionCliente);

   /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;

    contadorOcurrecias = Foccur32(fml,CLI_RUTX);

    for (i=0; i<= contadorOcurrecias-1; i++)
     {

        Fget32(fml, CLI_RUTX,            i, (char *)&cliClienteTributario.rut,   0);
        Fget32(fml, CLI_CLIENTE_TIN,     i, cliClienteTributario.tin,            0);

        stsCli=SqlCliEliminarClienteTributario(cliClienteTributario);
     }
	 	
    if (stsCli != DBA_SUCCESS)
     {
      userlog("error en eliminar cliente\n");
      Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error Al Eliminar Registro ...", 0);
     }
    else
     userlog("Eliminar  cliente CRS exitoso \n");

    TRX_COMMIT(transaccionCliente);
    tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
 
}	 

void CliTieneDeclCRS(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   long   rutCliente=0;
   char   tieneCRS[2];

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   tieneCRS[0]=0;

   Fget32(fml, CLI_RUTX, 0, (char *)&rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();


     sts = SqlCliTieneDeclaracionCRS(rutCliente, tieneCRS);
     if (sts != SQL_SUCCESS)
     {
        Fadd32(fml, HRM_MENSAJE_SERVICIO, "No se pudo recuperar la informacion de clienteCRS.", 0);

        if (transaccionGlobal == 0)
           tpabort(0L);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name), (char *)fml, 0L, 0L);
     }

     Fadd32(fml, CLI_CLIENTE_CRS, tieneCRS, 0);

     tpcommit(0L);
     tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}

/************************************************************************/
/* Objetivo: Validacion de limites de pago x RUT                        */
/* Autor   : Jaime Espindoila.            Fecha:  Junio 2017            */
/* Modifica:                              Fecha                         */
/************************************************************************/
void CliValLimPagRut(TPSVCINFO *rqst)
{
    FBFR32 *fml;
    long sts;
    int transaccionGlobal;
    short   codMovimiento=0;
    double  mtoMovimiento=0;
    long    rutCliente=0;
    long    codRespuesta=0;
    char    esMovimReversa[1 + 1];
    char    msgRespuesta[256+1];
    char    codError[3+1];
    short   codigoId;
    
   /******   Buffer de entrada   ******/
    fml = (FBFR32 *)rqst->data;  

printf("[INICIO] CliValLimPagRut\n");
Fprint32(fml);

    Fget32(fml, CCT_RUTCLIENTE,      0, (char *)&rutCliente, 0);
    Fget32(fml, CCT_TIPOMOVIMIENTO,  0, (char *)&codMovimiento, 0);
    Fget32(fml, CCT_MONTOMOVIMIENTO, 0, (char *)&mtoMovimiento, 0);
    Fget32(fml, CCT_CODIGO        , 0, (char *)&codigoId, 0);

    /******   Cuerpo del servicio   ******/
    transaccionGlobal = tpgetlev();
    TRX_BEGIN(transaccionGlobal);
    strcpy(esMovimReversa, "N");
    esMovimReversa[1] = '\0';
    memset((char *)&codError, 0x00, sizeof(codError));

printf("[INI] CliValLimPagRut - SqlVerSiMovimientoValidaLimites\n");
    sts = SqlVerSiMovimientoValidaLimites(codMovimiento, codigoId, esMovimReversa, &codRespuesta);
    if (sts != SQL_SUCCESS)
    {
        Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion SqlVerSiMovimientoValidaLimites.", 0);
        userlog("%s: Error en funcion SqlVerSiMovimientoValidaLimites.", rqst->name);
        TRX_ABORT(transaccionGlobal);
        tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
    }
    else
    {
printf("[*] CliValLimPagRut - codRespuesta = [%d]\n", codRespuesta);
       if (codRespuesta == 0) 
       {
           /* Movimiento esta en la lista de los que se debe revisar limites de pago */
            sts = SqlValidaLimitesPagosxRut(rutCliente, mtoMovimiento, esMovimReversa, codigoId, &codRespuesta, &codError);
            if (sts != SQL_SUCCESS)
            {
                Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion SqlValidaLimitesPagosxRut.", 0);
                userlog("%s: Error en funcion SqlValidaLimitesPagosxRut.", rqst->name);
                TRX_ABORT(transaccionGlobal);
                tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
            }
            else
            {
                if (codRespuesta == 0)
                { /* OK */
                   sprintf(msgRespuesta, "Aprueba Validacion limites de Pagos");
printf("[INI] CliValLimPagRut - SqlInsertaMovimLimitePagoCliente\n");
                   sts= SqlInsertaMovimLimitePagoCliente(rutCliente, codMovimiento, mtoMovimiento, esMovimReversa, codigoId);
                   if (sts != SQL_SUCCESS)
                   {
                       Fadd32(fml, HRM_MENSAJE_SERVICIO,"Error en funcion SqlInsertaMovimLimitePagoCliente.", 0);
                       userlog("%s: Error en funcion SqlInsertaMovimLimitePagoCliente.", rqst->name);
                       TRX_ABORT(transaccionGlobal);
                       tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
                   }
printf("[FIN] CliValLimPagRut - SqlInsertaMovimLimitePagoCliente\n");
                }                
                else
                {  /* No aprueba limite de pagos  */
                   sprintf(msgRespuesta, "Rechazado por Validacion limites de Pagos x RUT (%s)", codError);
                   codRespuesta= 3001; /* CCT_ERROR_LIMITEPAGOS_RUT_GENERAL; */
                }
                Fadd32(fml, CCT_CODIGO_ERROR,  codError, 0);
                Fadd32(fml, CCT_CODIGORESPUESTA,  (char *)&codRespuesta, 0);
                Fadd32(fml, CCT_MENSAJE_RESPUESTA, msgRespuesta, 0);
            }
printf("[FIN] CliValLimPagRut - SqlValidaLimitesPagosxRut\n");
       }
       else
       {
            /* Movimiento no esta en la lista de los que se debe revisar limites de pago, retora OK */
            codRespuesta= 0; /* OK */
            sprintf(msgRespuesta, "No requiere Validacion limites de Pagos");
            Fadd32(fml, CCT_CODIGORESPUESTA,  (char *)&codRespuesta, 0);
            Fadd32(fml, CCT_MENSAJE_RESPUESTA, msgRespuesta, 0);
       }
   }
printf("[FIN] CliValLimPagRut - SqlVerSiMovimientoValidaLimites\n");
   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}












/**************************************************************************************/
/* Objetivo: Insertar Alerta de Mensaje                                               */
/* Autor   : Alex Rojas                   Fecha: Julio 2015                           */
/* Modifica:                              Fecha:                                      */
/**************************************************************************************/
void CliInsMsjeAlert(TPSVCINFO *rqst)
{
   FBFR32 *fml;
   int transaccionGlobal;
   long sts;
   tCliMensajesAlerta mensajesAlerta;
   char  fechaProceso[8+1];

   memset(&mensajesAlerta,0,sizeof(tCliMensajesAlerta));

   /******   Buffer de entrada   ******/
   fml = (FBFR32 *)rqst->data;

   Fget32(fml, CLI_RUTX, 0, (char *)&mensajesAlerta.rutCliente, 0);

   /******   Cuerpo del servicio   ******/
   transaccionGlobal = tpgetlev();
   TRX_BEGIN(transaccionGlobal);

   sts = SqlCliRecuperarDatosAlertaCliente(&mensajesAlerta);
   if(sts != SQL_SUCCESS){
      Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al recuperar datos del cliente.", 0);
      tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   mensajesAlerta.tipoAlerta = 0;
   sprintf(mensajesAlerta.nombreProducto, "VACIO");

   if (strcmp(rqst->name, "CliInsAlertaTC") == 0)
   {
      mensajesAlerta.tipoAlerta = TIPO_ALERTA_AUMENTO_CUPO_TC_SMS;
      sprintf(mensajesAlerta.nombreProducto, "Aumento Cupo TC");
   }
   else if (strcmp(rqst->name, "CliInsAlertaLC") == 0)
   {
      mensajesAlerta.tipoAlerta = TIPO_ALERTA_AUMENTO_CUPO_LC_SMS;
      sprintf(mensajesAlerta.nombreProducto, "Aumento Cupo LC");
   }

   mensajesAlerta.estadoMensaje = 1;
   mensajesAlerta.numeroProducto = 1;
   strcpy(mensajesAlerta.fechaProceso, fechaProceso);
   mensajesAlerta.diasMora = 0;
   mensajesAlerta.edadCliente = 0;
   mensajesAlerta.numeroDocumento = 0;
   mensajesAlerta.numeroSerie[0] = '\0';
   mensajesAlerta.fechaVencimientoCuota[0] = '\0';
   mensajesAlerta.numeroCtaCte = 0;

   sts = SqlInsertarMensajeAlertaSMS(&mensajesAlerta);
   if(sts != SQL_SUCCESS){
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Mensaje ya insertado.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar Mensaje Alerta.", 0);

      tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }
   
   if (strcmp(rqst->name, "CliInsAlertaTC") == 0)
   {
      mensajesAlerta.tipoAlerta = TIPO_ALERTA_AUMENTO_CUPO_TC_EMAIL;
      sprintf(mensajesAlerta.nombreProducto, "Aumento Cupo TC");
   }
   else if (strcmp(rqst->name, "CliInsAlertaLC") == 0)
   {
      mensajesAlerta.tipoAlerta = TIPO_ALERTA_AUMENTO_CUPO_LC_EMAIL;
      sprintf(mensajesAlerta.nombreProducto, "Aumento Cupo LC");
   }
   
   sts = SqlInsertarMensajeAlertaEmail(&mensajesAlerta);
   if(sts != SQL_SUCCESS){
      if (sts == SQL_DUPLICATE_KEY)
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Mensaje ya insertado.", 0);
      else
         Fadd32(fml, HRM_MENSAJE_SERVICIO, "Error al insertar Mensaje Alerta.", 0);

      tpabort(0L);
      tpreturn(TPFAIL, ErrorServicio(sts, rqst->name),(char *)fml, 0L, 0L);
   }

   TRX_COMMIT(transaccionGlobal);
   tpreturn(TPSUCCESS, SRV_SUCCESS, (char *)fml, 0L, 0L);
}
