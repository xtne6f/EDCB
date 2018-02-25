using System;
using System.Security.AccessControl;
using System.Security.Principal;

namespace EpgTimer
{
    class KernelObjectSecurity : NativeObjectSecurity
    {
        class FakeSafeHandle : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            public FakeSafeHandle(IntPtr handle) : base(false)
            {
                SetHandle(handle);
            }

            protected override bool ReleaseHandle()
            {
                return true;
            }
        }

        public KernelObjectSecurity(IntPtr handle)
            : base(false, ResourceType.KernelObject, new FakeSafeHandle(handle), AccessControlSections.Access)
        {
        }

        public void AddAccessRule(KernelObjectAccessRule rule)
        {
            base.AddAccessRule(rule);
        }

        public void Persist(IntPtr handle)
        {
            Persist(new FakeSafeHandle(handle), AccessControlSections.Access);
        }

        public override Type AccessRightType
        {
            get { return typeof(int); }
        }

        public override AccessRule AccessRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited,
                                                     InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AccessControlType type)
        {
            return new KernelObjectAccessRule(identityReference, accessMask, type);
        }

        public override Type AccessRuleType
        {
            get { return typeof(KernelObjectAccessRule); }
        }

        public override AuditRule AuditRuleFactory(IdentityReference identityReference, int accessMask, bool isInherited,
                                                   InheritanceFlags inheritanceFlags, PropagationFlags propagationFlags, AuditFlags flags)
        {
            throw new NotImplementedException();
        }

        public override Type AuditRuleType
        {
            get { throw new NotImplementedException(); }
        }
    }

    class KernelObjectAccessRule : AccessRule
    {
        public KernelObjectAccessRule(IdentityReference identityReference, int accessMask, AccessControlType type)
            : base(identityReference, accessMask, false, InheritanceFlags.None, PropagationFlags.None, type)
        {
        }
    }
}
