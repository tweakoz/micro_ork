import os,sys
import platform
import subprocess
import shlex

###############################################################################

def capture(cmd):
    args = shlex.split(cmd)
    try:
      result = subprocess.check_output(args,universal_newlines=True,shell=False,stderr=sys.stdout.buffer)
    except subprocess.CalledProcessError as e:
      result = e.output
    return result

###############################################################################

class w8popen(subprocess.Popen):
    
    ##############################################
    
    def setup(self):
        self.IsDone = False
        self.ErrorOccured = False
        self.outtext = ""
        self.errtext = ""
        
    ##############################################

    def ioh(self):
       
        time.sleep(.01)
       
        stdout_data = self._recv( 'stdout', None )
        stderr_data = self._recv( 'stderr', None )
        
        if stdout_data != None:
            self.outtext = ""
            for ch in stdout_data:
                #print len( ch )
                if ch == '\n':
                #if len( self.outtext ) > 10:
                    #print "%d" % len( self.outtext )
                    print >> sys.stdout, self.outtext
                    self.outtext = ""
                else:
                    self.outtext = self.outtext + ch
             
        if stderr_data != None:
            self.errtext = ""
            for ch in stderr_data:
                if ch == '\n':
                    print >> sys.stderr, self.errtext
                    if self.errtext.rfind( "ERROR:" )!=-1:
                        self.ErrorOccured = True
                    self.errtext = ""
                else:
                    self.errtext = self.errtext + ch

    ##############################################
    
    def get_conn_maxsize(self, which, maxsize):
        if maxsize is None:
            maxsize = 1024
        elif maxsize < 1:
            maxsize = 1
        return getattr(self, which), maxsize
    
    ##############################################

    def _close(self, which):
        getattr(self, which).close()
        setattr(self, which, None)
        self.IsDone = True
        print >> sys.stdout, self.outtext
        print >> sys.stderr, self.errtext
    
###############################################################################

def spawner( cmdline ):
    ret = 0
    if IsWindows():
        p1 = w8popen("cmd /c %s" % cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
        #p1 = w8popen("cmd /c %s" % cmdline )
        p1.setup()
        while p1.IsDone == False:
            p1.ioh()
        p1.wait()
        if p1.ErrorOccured:
            ret = -3
        else:
            ret = p1.returncode
    elif False: #if IsCygwin():
        os.system( "cmd /c " + cmdline )
        ret = os.system( cmdline )
    else:
        ret = os.system( cmdline )
    
    print("ret<%s> %d\n" % ( cmdline, ret ))
    return ret
    
###############################################################################

def std_spawner( cmdline ):
    ret = 0
    if IsCygwin():
        os.system( "cmd /c " + cmdline )
        ret = os.system( cmdline )
    elif IsWindows():
        p1 = subprocess.Popen("cmd /c %s" % cmdline )
        p1.communicate()
        ret = p1.returncode
        
    #print "ret<%s> %d\n" % ( cmdline, ret )
    return ret

