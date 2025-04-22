# DISCLAIMER OF LIABILITY
# THIS IS SAMPLE SCRIPT. 
# NEITHER RECIPIENT NOR ANY CONTRIBUTORS SHALL HAVE ANY LIABILITY FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING WITHOUT LIMITATION LOST PROFITS), HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OR DISTRIBUTION OF THE PROGRAM OR THE EXERCISE OF ANY RIGHTS GRANTED
# HEREUNDER, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES

#!/bin/bash
#
# load.sh : a helper script for loading the drivers
#

# toggling bits for enabling scsi-mid layer logging

# enable sense data
#sysctl -w dev.scsi.logging_level=0x1000

# enable scanning debugging
sysctl -w dev.scsi.logging_level=0x1C0

# enable sense data and scanning
#sysctl -w dev.scsi.logging_level=0x11C0

# loading scsi mid layer
modprobe sg
modprobe sd_mod
modprobe scsi_transport_sas

# loading the driver
insmod mpi3mr.ko
