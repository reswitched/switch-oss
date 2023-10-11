﻿/*--------------------------------------------------------------------------------*
  Copyright (C)Nintendo. All rights reserved.

  These coded instructions, statements, and computer programs contain
  information of Nintendo and/or its licensed developers and are protected
  by national and international copyright laws.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *--------------------------------------------------------------------------------*/

extern "C"
{
    void* nndetailRoGetRoDataStart()
    {
        extern unsigned char __rodata_start;
        return &__rodata_start;
    }

    void* nndetailRoGetRoDataEnd()
    {
        extern unsigned char __rodata_end;
        return &__rodata_end;
    }
}
